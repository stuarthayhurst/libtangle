#include <iostream>

#include "thread.hpp"

#include "debug.hpp"
#include "logging.hpp"

namespace tangle {
  namespace thread {
    //Return the number of hardware threads available
    unsigned int getHardwareThreadCount() {
      return internal::getHardwareThreadCount();
    }

    //Return the number of threads in the given pool
    unsigned int getThreadPoolSize(void* threadPool) {
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
     - If jobs in the queue may more submit work, they must be completed before calling this
     - This will block until the queued jobs complete
    */
    void destroyThreadPool(void* threadPool) {
      internal::destroyThreadPool(threadPool);
    }

    /*
     - Submit a job to the given thread pool, with a user-provided pointer
       - userPtr may be a nullptr
     - Do not submit jobs that block conditionally on other jobs
    */
    void submitWork(TangleWork work, void* userPtr, void* threadPool) {
      internal::submitWork(work, userPtr, nullptr, threadPool);
    }

    /*
     - Submit a job to the given thread pool, with a user-provided pointer and group
       - group should either be a nullptr, or an TangleGroup{0}
         - A group can be used between multiple calls, but waiting on it will block
           until all work in the group is done
       - userPtr may be a nullptr
     - Do not submit jobs that block conditionally on other jobs
    */
    void submitWork(TangleWork work, void* userPtr, TangleGroup* group, void* threadPool) {
      internal::submitWork(work, userPtr, group, threadPool);
    }

    /*
     - Submit multiple jobs to the given thread pool, with a user-provided buffer and group
       - userBuffer should either be a nullptr, or an array of data to be split between jobs
         - Each job will receive a section according to (userBuffer + job index * stride)
         - stride should be the size of each section to give to a job, in bytes
       - group should either be a nullptr, or an TangleGroup{0}
       - submitGroup should either be a nullptr, or an TangleGroup{0}
       - jobCount specifies how many times to submit the job
     - Jobs are submitted asynchronously, waiting on submitGroup for 1 job can be used
       to wait for the submit to be complete
       - Waiting on submitGroup or group must be done before destroying the thread pool
       - This may be a while, use submitMultipleSync() instead of immediately waiting
     - Do not submit jobs that block conditionally on other jobs
    */
    void submitMultiple(TangleWork work, void* userBuffer, int stride,
                        TangleGroup* group, unsigned int jobCount,
                        TangleGroup* submitGroup, void* threadPool) {
      //Set stride to 0 when no data is passed
      if (userBuffer == nullptr) {
        stride = 0;
      }

      internal::submitMultiple(work, userBuffer, stride, group, jobCount, submitGroup, threadPool);
    }

    //Synchronous version of submitMultiple()
    void submitMultipleSync(TangleWork work, void* userBuffer, int stride,
                            TangleGroup* group, unsigned int jobCount, void* threadPool) {
      //Set stride to 0 when no data is passed
      if (userBuffer == nullptr) {
        stride = 0;
      }

      internal::submitMultipleSync(work, userBuffer, stride, group, jobCount, threadPool);
    }

    /*
     - Wait for a group to be finished
     - jobCount determines how many jobs to wait for
       - If less than jobCount jobs have been given the group, this will block forever
       - It doesn't matter if the jobs have already finished
    */
    void waitGroupComplete(TangleGroup* group, unsigned int jobCount) {
      if (group != nullptr) {
        internal::waitGroupComplete(group, jobCount);
      } else {
        tangleInternalDebug << "Group is a nullptr, skipping wait" << std::endl;
      }
    }

    /*
     - Check if at least one item of a group has finished
       - May spuriously fail, returning false when work had finished
     - Acts like synchronisation if successful, decreasing the group's counter
       - A second call to a group with 1 complete work item would return false
       - Using waitGroupComplete() at this point would block
     - If unsuccessful, nothing in the group is modified
    */
    bool isSingleWorkComplete(TangleGroup* group) {
      if (group != nullptr) {
        return internal::isSingleWorkComplete(group);
      }

      tangleInternalDebug << "Group is a nullptr, skipping check" << std::endl;
      return false;
    }

    /*
     - Return the number of unfinished jobs in a group
     - Successive calls should use the remaining jobs returned as the job count
       - Subtract any synchronised / successfully queried jobs from this too
     - Remaining work may be overestimated, but never underestimated
     - Acts like synchronisation if successful, decreasing the group's counter
     - If unsuccessful, nothing in the group is modified
    */
    unsigned int getRemainingWork(TangleGroup* group, unsigned int jobCount) {
      if (group != nullptr) {
        return internal::getRemainingWork(group, jobCount);
      }

      tangleInternalDebug << "Group is a nullptr, skipping query" << std::endl;
      return jobCount;
    }

    /*
     - Block the given pool from starting new jobs
     - Returns once all threads are blocked
     - This isn't thread safe, and must never be called from a job
    */
    void blockThreads(void* threadPool) {
      internal::blockThreads(threadPool);
    }

    /*
     - Allow the pool to start new jobs again
     - Returns once threads are have woken up
     - This isn't thread safe, and must never be called from a job
    */
    void unblockThreads(void* threadPool) {
      internal::unblockThreads(threadPool);
    }

    /*
     - Wait until all work in the pool as of the call is finished
       - If a job submits more work while executing, the extra work won't be waited for
       - This includes submitMultiple(), which submits a job to submit the actual jobs
     - This isn't thread safe, and must never be called from a job
    */
    void finishWork(void* threadPool) {
      internal::finishWork(threadPool);
    }
  }
}
