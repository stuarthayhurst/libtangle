#include <iostream>

#include "thread.hpp"

#include "group.hpp"
#include "internal/threadPool.hpp"
#include "logging.hpp"

/*
 - Connects exposed functions to the internal thread pool functions
 - Adds some additional checks and convenience functions
*/

namespace tangle {
  namespace thread {
    //Return the number of hardware threads available
    unsigned int getHardwareThreadCount() {
      return internal::getHardwareThreadCount();
    }

    //Return the expected number of threads for a thread pool
    unsigned int getExpectedThreadPoolSize(unsigned int threadCount) {
      return internal::getExpectedThreadPoolSize(threadCount);
    }

    //Return the number of threads in the given pool
    unsigned int getThreadPoolSize(void* const threadPool) {
      return internal::getThreadPoolSize(threadPool);
    }

    /*
     - Create a new thread pool
     - Use destroyThreadPool() to clean up afterwards
    */
    void* createThreadPoolInstance(unsigned int threadCount) {
      void* const threadPool = internal::createThreadPoolInstance(threadCount);
      if (threadPool == nullptr) {
        tangle::utils::warning << "Failed to create a new thread pool instance" << std::endl;
      }

      return threadPool;
    }

    /*
     - Destroy the given thread pool
     - Must only be called once per thread pool
     - If jobs in the queue may submit more work, they must be completed before calling this
     - This will block until the queued jobs complete
    */
    void destroyThreadPool(void* const threadPool) {
      internal::destroyThreadPool(threadPool);
    }

    /*
     - Submit a job to the given thread pool, with a user-provided pointer
       - userPtr may be NO_USERPTR (nullptr)
     - Do not submit jobs that block conditionally on other jobs
    */
    void submitWork(TangleWork work, void* const userPtr, void* const threadPool) {
      internal::submitWork(work, userPtr, nullptr, threadPool);
    }

    /*
     - Submit a job to the given thread pool, with a user-provided pointer and group
       - group should either be NO_GROUP (nullptr), or a TangleGroup{0}
         - A group can be used between multiple calls, but waiting on it will block
           until all work in the group is done
       - userPtr may be NO_USERPTR (nullptr)
     - Do not submit jobs that block conditionally on other jobs
    */
    void submitWork(TangleWork work, void* const userPtr,
                    TangleGroup* const group, void* const threadPool) {
      internal::submitWork(work, userPtr, group, threadPool);
    }

    /*
     - Submit multiple jobs to the given thread pool, with a user-provided buffer and group
       - userBuffer should either be NO_USERBUFFER (nullptr), or an array of data
         to be split between jobs
         - Each job will receive a section according to (userBuffer + job index * stride)
         - stride should be the size of each section to give to a job, in bytes
       - group should either be NO_GROUP (nullptr), or a TangleGroup{0}
       - submitGroup should either be NO_GROUP (nullptr), or a TangleGroup{0}
       - jobCount specifies how many times to submit the job
     - Jobs are submitted asynchronously, waiting on submitGroup for 1 job can be used
       to wait for the submit to be complete
       - Waiting on submitGroup or group must be done before destroying the thread pool
       - This may be a while, use submitMultipleSync() instead of immediately waiting
     - Do not submit jobs that block conditionally on other jobs
    */
    void submitMultiple(TangleWork work, void* const userBuffer, int stride,
                        TangleGroup* const group, unsigned int jobCount,
                        TangleGroup* const submitGroup, void* const threadPool) {
      //Set stride to 0 when no data is passed
      if (userBuffer == nullptr) {
        stride = 0;
      }

      internal::submitMultiple(work, userBuffer, stride, group, jobCount, submitGroup, threadPool);
    }

    /*
     - Same as the standard submitMultiple(), but all jobs share a user-provided pointer
       instead of a buffer
    */
    void submitMultiple(TangleWork work, void* const userPtr,
                        TangleGroup* const group, unsigned int jobCount,
                        TangleGroup* const submitGroup, void* const threadPool) {
      internal::submitMultiple(work, userPtr, 0, group, jobCount, submitGroup, threadPool);
    }

    /*
     - Synchronous version of submitMultiple()
     - Jobs are submitted immediately, instead of waiting for the thread pool
       to get around to the deferred submission job
     - This is more efficient than submitMultiple() and immediately syncing
    */
    void submitMultipleSync(TangleWork work, void* const userBuffer, int stride,
                            TangleGroup* const group, unsigned int jobCount,
                            void* const threadPool) {
      //Set stride to 0 when no data is passed
      if (userBuffer == nullptr) {
        stride = 0;
      }

      internal::submitMultipleSync(work, userBuffer, stride, group, jobCount, threadPool);
    }

    /*
     - Same as the synchronous submitMultiple(), but all jobs share a user-provided pointer
       instead of a buffer
    */
    void submitMultipleSync(TangleWork work, void* const userPtr,
                            TangleGroup* const group, unsigned int jobCount,
                            void* const threadPool) {
      internal::submitMultipleSync(work, userPtr, 0, group, jobCount, threadPool);
    }

    /*
     - Block the given pool from starting new jobs
     - Returns once all threads are blocked
     - This isn't thread safe, and must never be called from a job
    */
    void blockThreads(void* const threadPool) {
      internal::blockThreads(threadPool);
    }

    /*
     - Allow the pool to start new jobs again
     - Returns once threads are have woken up
     - This isn't thread safe, and must never be called from a job
    */
    void unblockThreads(void* const threadPool) {
      internal::unblockThreads(threadPool);
    }

    /*
     - Wait until all work in the pool as of the call is finished
       - If a job submits more work while executing, the extra work won't be waited for
       - This includes submitMultiple(), which submits a job to submit the actual jobs
     - This isn't thread safe, and must never be called from a job
    */
    void finishWork(void* const threadPool) {
      internal::finishWork(threadPool);
    }
  }
}
