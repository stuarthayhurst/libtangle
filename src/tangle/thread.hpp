#ifndef INTERNALTHREAD
#define INTERNALTHREAD

#include "visibility.hpp"

//Include public interface
#include "../include/tangle/thread.hpp" // IWYU pragma: export

namespace TANGLE_INTERNAL tangle {
  namespace thread {
    namespace internal {
      unsigned int getHardwareThreadCount();
      unsigned int getThreadPoolSize();

      bool createThreadPool(unsigned int threadCount);
      void destroyThreadPool();

      void submitWork(TangleWork work, void* userPtr, TangleGroup* group);
      void submitMultiple(TangleWork work, void* userBuffer, int stride,
                          TangleGroup* group, unsigned int newJobs,
                          TangleGroup* submitGroup);
      void submitMultipleSync(TangleWork work, void* userBuffer, int stride,
                              TangleGroup* group, unsigned int newJobs);

      void waitGroupComplete(TangleGroup* group, unsigned int jobCount);
      bool isSingleWorkComplete(TangleGroup* group);
      unsigned int getRemainingWork(TangleGroup* group, unsigned int jobCount);

      void blockThreads();
      void unblockThreads();
      void finishWork();
    }
  }
}

#endif
