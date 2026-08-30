#include <iostream>

#include "monopool.hpp"

#if ENABLE_MONO_POOL == 1

#include "../group.hpp"
#include "../logging.hpp"
#include "../thread.hpp"
#include "../internal/debug.hpp"
#include "../internal/threadPool.hpp"

/*
 - Implements the legacy single thread pool API
 - Tracks total thread pool users to create and destroy a single thread pool as required
 - This is only provided for compatibility, and should generally be avoided
*/

namespace tangle {
  namespace thread {
    namespace {
      void* threadPool = nullptr;
      unsigned int poolUsers = 0;
    }

    /*
     - Return the number of threads in the thread pool
     - Returns 0 if it doesn't exist
    */
    unsigned int getThreadPoolSize() {
      if (poolUsers == 0) {
        return 0;
      }

      return internal::getThreadPoolSize(threadPool);
    }

    /*
     - Create or increase the reference counter on the thread pool
     - Use destroyThreadPool() to clean up afterwards
     - Returns false if no thread pool exists or was created, otherwise true
    */
    bool createThreadPool(unsigned int threadCount) {
      bool exists = true;
      if (poolUsers == 0) {
        threadPool = internal::createThreadPoolInstance(threadCount);
        if (threadPool == nullptr) {
          exists = false;
        }
      }

      poolUsers++;
      return exists;
    }

    /*
     - Destroy or exit the current thread pool
     - Must be called once per creation / connection
     - If jobs in the queue may submit more work, they must be completed before calling this
     - This will only block until the jobs complete if it's the final user of the pool
    */
    void destroyThreadPool() {
      if (poolUsers == 0) {
        tangle::utils::warning << "Attempted to destroy a thread pool before creation, ignoring" \
                               << std::endl;
        return;
      }

      poolUsers--;
      if (poolUsers == 0) {
        internal::destroyThreadPool(threadPool);
      } else {
        tangleInternalDebug << "Skipping thread pool destruction, " \
                            << poolUsers << " users remain" << std::endl;
      }
    }

    /*
     - Submit a job to the thread pool, with a user-provided pointer
       - userPtr may be a nullptr
     - createThreadPool() must be called before using this
     - Do not submit jobs that block conditionally on other jobs
    */
    void submitWork(TangleWork work, void* const userPtr) {
      internal::submitWork(work, userPtr, nullptr, threadPool);
    }

    /*
     - Submit a job to the thread pool, with a user-provided pointer and group
       - group should either be a nullptr, or a TangleGroup{0}
         - A group can be used between multiple calls, but waiting on it will block
           until all work in the group is done
       - userPtr may be a nullptr
     - createThreadPool() must be called before using this
     - Do not submit jobs that block conditionally on other jobs
    */
    void submitWork(TangleWork work, void* const userPtr, TangleGroup* const group) {
      internal::submitWork(work, userPtr, group, threadPool);
    }

    /*
     - Submit multiple jobs to the thread pool, with a user-provided buffer and group
       - userBuffer should either be a nullptr, or an array of data to be split between jobs
         - Each job will receive a section according to (userBuffer + job index * stride)
         - stride should be the size of each section to give to a job, in bytes
       - group should either be a nullptr, or a TangleGroup{0}
       - submitGroup should either be a nullptr, or a TangleGroup{0}
       - jobCount specifies how many times to submit the job
     - Jobs are submitted asynchronously, waiting on submitGroup for 1 job can be used
       to wait for the submit to be complete
       - Waiting on submitGroup or group must be done before destroying the thread pool
       - This may be a while, use submitMultipleSync() instead of immediately waiting
     - createThreadPool() must be called before using this
     - Do not submit jobs that block conditionally on other jobs
    */
    void submitMultiple(TangleWork work, void* const userBuffer, int stride,
                        TangleGroup* const group, unsigned int jobCount,
                        TangleGroup* const submitGroup) {
      //Set stride to 0 when no data is passed
      if (userBuffer == nullptr) {
        stride = 0;
      }

      internal::submitMultiple(work, userBuffer, stride, group, jobCount,
                               submitGroup, threadPool);
    }

    /*
     - Same as the standard submitMultiple(), but all jobs share a user-provided pointer
       instead of a buffer
    */
    void submitMultiple(TangleWork work, void* const userPtr,
                        TangleGroup* const group, unsigned int jobCount,
                        TangleGroup* const submitGroup) {
      internal::submitMultiple(work, userPtr, 0, group, jobCount, submitGroup,
                               threadPool);
    }

    //Synchronous version of submitMultiple()
    void submitMultipleSync(TangleWork work, void* const userBuffer, int stride,
                            TangleGroup* const group, unsigned int jobCount) {
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
                            TangleGroup* const group, unsigned int jobCount) {
      internal::submitMultipleSync(work, userPtr, 0, group, jobCount, threadPool);
    }

    /*
     - Block the pool from starting new jobs
     - Returns once all threads are blocked
     - This isn't thread safe, and must never be called from a job
    */
    void blockThreads() {
      if (poolUsers != 0) {
        internal::blockThreads(threadPool);
      }
    }

    /*
     - Allow the pool to start new jobs again
     - Returns once threads are have woken up
     - This isn't thread safe, and must never be called from a job
    */
    void unblockThreads() {
      if (poolUsers != 0) {
        internal::unblockThreads(threadPool);
      }
    }

    /*
     - Wait until all work in the pool as of the call is finished
       - If a job submits more work while executing, the extra work won't be waited for
       - This includes submitMultiple(), which submits a job to submit the actual jobs
     - This isn't thread safe, and must never be called from a job
    */
    void finishWork() {
      if (poolUsers != 0) {
        internal::finishWork(threadPool);
      }
    }
  }
}

#endif
