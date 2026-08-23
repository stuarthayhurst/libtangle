#include <atomic>
#include <barrier>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <system_error>
#include <thread>

#include <unistd.h>

#include "threadPool.hpp"

#include "queue.hpp"

#include "../group.hpp"
#include "../internal/debug.hpp"
#include "../logging.hpp"
#include "../thread.hpp"

/*
 - Implements the core thread pool functionality internally
 - Each thread pool has multiple queues, cycling between them for new jobs
   - The requested thread count is capped at 512
   - The adjusted thread count is rounded to nearest power of 2 greater than or equal to it, then
     doubled to get the queue lane count
 - Each queue isn't lock free, but jobs can be submitted as a batch to mitigate this
*/

static constexpr unsigned int MAX_THREADS = 512;

namespace tangle {
  namespace thread {
    namespace internal {
      namespace {
        //Thread pool data
        struct ThreadPool {
          //Data for the threads working from workQueues
          unsigned int poolThreadCount = 0;
          std::thread* threadArray;
          std::atomic<bool> stayAlive = false;

          //Barrier to synchronise all threads
          std::barrier<>* threadSyncBarrier;

          //Trigger, flag and barrier to block all threads, then communicate completion
          std::atomic<bool> threadBlockTrigger = false;
          bool threadsBlocked = false;
          std::barrier<>* threadBlockBarrier;
          std::barrier<>* threadUnblockBarrier;

          /*
           - Data for the work queues
           - Treat workQueues as a circular buffer, with separate heads
             for job pushes and pops
          */
          tangle::internal::WorkQueue* workQueues;
          unsigned int queueLaneCount = 0;
          unsigned int laneAssignMask = 0;
          std::atomic<uintmax_t> nextQueueRead = 0;
          std::atomic<uintmax_t> nextQueueWrite = 0;
        };
      }

      namespace {
        void runWorker(ThreadPool* threadPool) {
          //Ask the system for the thread ID, since it's more useful for debugging
          tangleInternalDebug << "Started worker thread (ID " << gettid() \
                              << ")" << std::endl;

          tangle::internal::WorkItem workItem = {
            .work = nullptr, .userPtr = nullptr, .group = nullptr
          };
          while (threadPool->stayAlive) {
            //Wait for a job to be available, then return it
            const uintmax_t targetQueue = (threadPool->nextQueueRead++) & threadPool->laneAssignMask;
            threadPool->workQueues[targetQueue].pop(&workItem);

            //Block the thread when instructed to, wait to release it
            if (threadPool->threadBlockTrigger) {
              threadPool->threadBlockBarrier->arrive_and_wait();
              threadPool->threadBlockTrigger.wait(true);

              //Mark thread as unblocked as it resumes
              (void)threadPool->threadUnblockBarrier->arrive();
            }

            /*
             - Execute the work or sleep
             - workItem.work may be nullptr to wake the threads up
            */
            if (workItem.work != nullptr) {
              workItem.work(workItem.userPtr);

              //Update the group semaphore, if given
              if (workItem.group != nullptr) {
                workItem.group->release();
              }
            }
          }
        }

        void wakeThreads(ThreadPool* threadPool) {
          for (unsigned int i = 0; i < threadPool->poolThreadCount; i++) {
            internal::submitWork(nullptr, nullptr, nullptr, threadPool);
          }
        }

        //Simple job to synchronise threads
        void finishSyncJob(void* threadPoolPtr) {
          ThreadPool* const threadPool = (ThreadPool*)threadPoolPtr;
          threadPool->threadSyncBarrier->arrive_and_wait();
        }
      }

      //Helpers for asynchronous submitMultiple()
      namespace {
        struct SubmitData {
          TangleWork work;
          void* userBuffer;
          TangleGroup* group;
          int stride;
          unsigned int jobCount;
          ThreadPool* threadPool;
        };

        void submitMultipleJob(void* rawSubmitData) {
          SubmitData* const submitData = (SubmitData*)rawSubmitData;
          ThreadPool* const threadPool = submitData->threadPool;

          //Every queue gets at least baseBatchSize jobs
          const unsigned int baseBatchSize = submitData->jobCount / threadPool->queueLaneCount;

          //Add the base amount of work to each queue without touching the atomic index
          if (baseBatchSize > 0) {
            const std::size_t jobSize = std::size_t(baseBatchSize) * submitData->stride;
            for (unsigned int i = 0; i < threadPool->queueLaneCount; i++) {
              threadPool->workQueues[i].pushMultiple(submitData->work, submitData->userBuffer,
                                                     submitData->stride, submitData->group,
                                                     baseBatchSize);
              submitData->userBuffer = (char*)submitData->userBuffer + jobSize;
            }
          }

          //Add the remaining work
          const unsigned int remainingJobs = submitData->jobCount - (baseBatchSize * threadPool->queueLaneCount);
          for (unsigned int i = 0; i < remainingJobs; i++) {
            const uintmax_t targetQueue = (threadPool->nextQueueWrite++) & threadPool->laneAssignMask;
            threadPool->workQueues[targetQueue].push(submitData->work, (char*)submitData->userBuffer,
                                                     submitData->group);
            submitData->userBuffer = (char*)submitData->userBuffer + submitData->stride;
          }

          delete submitData;
        }
      }

      unsigned int getHardwareThreadCount() {
        return std::thread::hardware_concurrency();
      }

      //Calculate the real number of threads to be used
      unsigned int getExpectedThreadPoolSize(unsigned int threadCount) {
        //Default to creating a worker thread for every hardware thread
        if (threadCount == 0) {
          threadCount = getHardwareThreadCount();
        }

        //Cap at configured thread limit
        threadCount = (threadCount > MAX_THREADS) ? MAX_THREADS : threadCount;

        return threadCount;
      }

      unsigned int getThreadPoolSize(void* threadPoolPtr) {
        return ((ThreadPool*)threadPoolPtr)->poolThreadCount;
      }

      //Submit a job to the next queue of a given thread pool
      void submitWork(TangleWork work, void* userPtr, TangleGroup* group, void* threadPoolPtr) {
        ThreadPool* const threadPool = (ThreadPool*)threadPoolPtr;

        //Add work to the next queue
        const uintmax_t targetQueue = (threadPool->nextQueueWrite++) & threadPool->laneAssignMask;
        threadPool->workQueues[targetQueue].push(work, userPtr, group);
      }

      /*
       - Submit a job that submits the actual work when executed
       - Submitting the actual jobs asynchronously returns faster, allowing
         the overhead to be mitigated by useful work
       - All work is submitted to the specified thread pool
      */
      void submitMultiple(TangleWork work, void* userBuffer, int stride,
                          TangleGroup* group, unsigned int newJobs,
                          TangleGroup* submitGroup, void* threadPoolPtr) {
        ThreadPool* const threadPool = (ThreadPool*)threadPoolPtr;

        //Pack the data into the expected format and submit the job
        SubmitData* const dataPtr = new SubmitData{
          .work = work, .userBuffer = userBuffer, .group = group,
          .stride = stride, .jobCount = newJobs, .threadPool = threadPool};
        internal::submitWork(submitMultipleJob, dataPtr, submitGroup, threadPool);
      }

      //Synchronous version of submitMultiple()
      void submitMultipleSync(TangleWork work, void* userBuffer,
                              int stride, TangleGroup* group, unsigned int newJobs,
                              void* threadPoolPtr) {
        ThreadPool* const threadPool = (ThreadPool*)threadPoolPtr;

        //Pack the data into the expected format and just execute the job immediately
        SubmitData* const dataPtr = new SubmitData{
          .work = work, .userBuffer = userBuffer, .group = group,
          .stride = stride, .jobCount = newJobs, .threadPool = threadPool};
        submitMultipleJob(dataPtr);
      }

      /*
       - Create and return a thread pool of the requested size
       - Use the number of threads rounded to the first power of 2 greater than or
         equal to it then double it as the lane count
      */
      void* createThreadPoolInstance(unsigned int threadCount) {
        //Create a new thread pool
        ThreadPool* const threadPool = new ThreadPool;

        //Calculate the real thread count
        threadCount = getExpectedThreadPoolSize(threadCount);

        //Allocate memory for the pool
        tangleInternalDebug << "Creating thread pool with " << threadCount \
                            << " thread(s)" << std::endl;
        threadPool->threadArray = new std::thread[threadCount];
        if (threadPool->threadArray == nullptr) {
          delete threadPool;
          return nullptr;
        }

        //Round thread count up to nearest power of 2 and double it to decide lane count
        threadPool->queueLaneCount = std::bit_ceil(threadCount) * 2;
        threadPool->laneAssignMask = threadPool->queueLaneCount - 1;

        //Create the queues
        threadPool->nextQueueRead = 0;
        threadPool->nextQueueWrite = 0;
        threadPool->workQueues = new tangle::internal::WorkQueue[threadPool->queueLaneCount];

        //Prepare thread finish barrier
        threadPool->threadSyncBarrier = new std::barrier{threadCount};

        //Prepare thread block syncs
        threadPool->threadBlockBarrier = new std::barrier{threadCount + 1};
        threadPool->threadUnblockBarrier = new std::barrier{threadCount + 1};
        threadPool->threadsBlocked = false;
        threadPool->threadBlockTrigger = false;

        //Create the threads for the pool
        threadPool->stayAlive = true;
        threadPool->poolThreadCount = threadCount;
        for (unsigned int i = 0; i < threadPool->poolThreadCount; i++) {
          threadPool->threadArray[i] = std::thread(runWorker, threadPool);
        }

        return threadPool;
      }

      /*
       - Instruct all threads of a given thread pool to block after their current job
       - Create a fake job for each thread in case they're asleep
       - Return once the threads are all blocked
      */
      void blockThreads(void* threadPoolPtr) {
        ThreadPool* const threadPool = (ThreadPool*)threadPoolPtr;

        if (!threadPool->threadsBlocked) {
          //Instruct thread to block
          threadPool->threadBlockTrigger = true;

          //Threads need to be woken up, in case they're waiting for work
          wakeThreads(threadPool);

          //Wait for threads to block
          threadPool->threadBlockBarrier->arrive_and_wait();
          threadPool->threadsBlocked = true;
        }
      }

      /*
       - Instruct all threads of a given thread pool to resume execution
       - Return as soon as all threads have woken up
      */
      void unblockThreads(void* threadPoolPtr) {
        ThreadPool* const threadPool = (ThreadPool*)threadPoolPtr;

        if (threadPool->threadsBlocked) {
          //Instruct threads to unblock
          threadPool->threadBlockTrigger = false;
          threadPool->threadBlockTrigger.notify_all();

          //Wait for all thread to unblock, then clean up and return
          threadPool->threadUnblockBarrier->arrive_and_wait();
          threadPool->threadsBlocked = false;
        }
      }

      /*
       - Complete all work already queued in a given thread pool
       - Return when the work has finished
      */
      void finishWork(void* threadPoolPtr) {
        ThreadPool* const threadPool = (ThreadPool*)threadPoolPtr;

        TangleGroup group{0};
        for (unsigned int i = 0; i < threadPool->poolThreadCount; i++) {
          internal::submitWork(finishSyncJob, threadPoolPtr, &group, threadPool);
        }

        internal::waitGroupComplete(&group, threadPool->poolThreadCount);
      }

      /*
       - Finish work already in the thread pool
       - Destroy the thread pool
      */
      void destroyThreadPool(void* threadPoolPtr) {
        ThreadPool* const threadPool = (ThreadPool*)threadPoolPtr;
        tangleInternalDebug << "Destroying thread pool" << std::endl;

        if (threadPool->threadsBlocked) {
          tangle::utils::warning << "Attempting to destroy thread pool while blocked" \
                                 << std::endl;
        }

        //Finish existing work
        internal::finishWork(threadPool);

        //Block threads, instruct them to die then unblock
        internal::blockThreads(threadPool);
        threadPool->stayAlive = false;
        internal::unblockThreads(threadPool);

        //Wait until all threads are done
        for (unsigned int i = 0; i < threadPool->poolThreadCount; i++) {
          try {
            threadPool->threadArray[i].join();
          } catch (const std::system_error&) {
            tangle::utils::warning << "Failed to join thread " << i \
                                   << " while destroying thread pool" << std::endl;
          }
        }

        //Delete thread pool components
        delete [] threadPool->workQueues;
        delete [] threadPool->threadArray;
        delete threadPool->threadSyncBarrier;
        delete threadPool->threadBlockBarrier;
        delete threadPool->threadUnblockBarrier;

        //Delete the thread pool data container
        delete threadPool;
      }
    }
  }
}
