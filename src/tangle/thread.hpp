#ifndef INTERNALTHREAD
#define INTERNALTHREAD

#include "visibility.hpp"

//Include public interface
#include "../include/tangle/thread.hpp" // IWYU pragma: export

namespace TANGLE_INTERNAL tangle {
  namespace thread {
    namespace internal {
      unsigned int getHardwareThreadCount();
      unsigned int getThreadPoolSize(void* threadPoolPtr);

      void* createThreadPoolInstance(unsigned int threadCount);
      void destroyThreadPool(void* threadPoolPtr);

      void submitWork(TangleWork work, void* userPtr, TangleGroup* group, void* threadPoolPtr);
      void submitMultiple(TangleWork work, void* userBuffer, int stride,
                          TangleGroup* group, unsigned int newJobs,
                          TangleGroup* submitGroup, void* threadPoolPtr);
      void submitMultipleSync(TangleWork work, void* userBuffer, int stride,
                              TangleGroup* group, unsigned int newJobs, void* threadPoolPtr);

      void waitGroupComplete(TangleGroup* group, unsigned int jobCount);
      bool isSingleWorkComplete(TangleGroup* group);
      unsigned int getRemainingWork(TangleGroup* group, unsigned int jobCount);

      void blockThreads(void* threadPoolPtr);
      void unblockThreads(void* threadPoolPtr);
      void finishWork(void* threadPoolPtr);
    }
  }
}

#endif
