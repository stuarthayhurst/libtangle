#ifndef INTERNALTHREADMONO
#define INTERNALTHREADMONO

//#include "../visibility.hpp"

//#include "../thread.hpp"

//Include public interface
#include "../../include/tangle/compat/monopool.hpp" // IWYU pragma: export
/*
namespace TANGLE_INTERNAL tangle {
  namespace thread {
    namespace internal {
      unsigned int getThreadPoolSize();

      bool createThreadPool(unsigned int threadCount);
      void destroyThreadPool();

      void submitWork(TangleWork work, void* userPtr, TangleGroup* group);
      void submitMultiple(TangleWork work, void* userBuffer, int stride,
                          TangleGroup* group, unsigned int newJobs,
                          TangleGroup* submitGroup);
      void submitMultipleSync(TangleWork work, void* userBuffer, int stride,
                              TangleGroup* group, unsigned int newJobs);

      void blockThreads();
      void unblockThreads();
      void finishWork();
    }
  }
}
*/
#endif
