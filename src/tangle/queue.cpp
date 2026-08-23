#include <cstddef>

#include "queue.hpp"

#include "group.hpp"
#include "thread.hpp"

/*
 - Implements a thread-safe queue to store and retrieve jobs from
 - Job pushes and pops all use the mutex, job pops wait on the semaphore
   for work to be available
 - It's not lock free, but jobs can be submitted as a batch to mitigate this
*/

namespace tangle {
  namespace internal {
    void WorkQueue::push(TangleWork work, void* userPtr, TangleGroup* group) {
      this->queueLock.lock();

      this->queue.push({.work = work, .userPtr = userPtr, .group = group});

      this->queueLock.unlock();
      this->jobCount.release();
    }

    void WorkQueue::pushMultiple(TangleWork work, void* userBuffer, int stride,
                                 TangleGroup* group, unsigned int count) {
      this->queueLock.lock();

      //Add multiple jobs in a single pass
      for (std::size_t i = 0; i < count; i++) {
        this->queue.push({.work = work, .userPtr = (char*)userBuffer + (i * stride),
                          .group = group});
      }

      this->queueLock.unlock();
      this->jobCount.release(count);
    }

    void WorkQueue::pop(WorkItem* workItemPtr) {
      this->jobCount.acquire();
      this->queueLock.lock();

      *workItemPtr = this->queue.front();
      this->queue.pop();

      this->queueLock.unlock();
    }
  }
}
