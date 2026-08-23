#ifndef INTERNALQUEUE
#define INTERNALQUEUE

#include <cstdint>
#include <limits>
#include <mutex>
#include <queue>
#include <semaphore>

#include "../group.hpp"
#include "../thread.hpp"
#include "../visibility.hpp"

namespace TANGLE_INTERNAL tangle {
  namespace internal {
    struct WorkItem {
      TangleWork work;
      void* userPtr;
      TangleGroup* group;
    };

    class WorkQueue {
    private:
      std::queue<WorkItem> queue;
      std::mutex queueLock;
      std::counting_semaphore<std::numeric_limits<int32_t>::max()> jobCount{0};

    public:
      void push(TangleWork work, void* userPtr, TangleGroup* group);
      void pushMultiple(TangleWork work, void* userBuffer, int stride,
                        TangleGroup* group, unsigned int count);
      void pop(WorkItem* workItemPtr);
    };
  }
}

#endif
