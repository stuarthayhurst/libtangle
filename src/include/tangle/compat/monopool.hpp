#ifndef TANGLETHREADMONO
#define TANGLETHREADMONO

//Enable legacy single thread pool API if not explicitly disabled
#ifndef ENABLE_MONO_POOL
//NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ENABLE_MONO_POOL 1
#endif

#if ENABLE_MONO_POOL == 1

#include "../group.hpp"
#include "../thread.hpp"
#include "../visibility.hpp"

namespace TANGLE_EXPOSED tangle {
  namespace thread {
    unsigned int getThreadPoolSize();

    bool createThreadPool(unsigned int threadCount);
    void destroyThreadPool();

    void submitWork(TangleWork work, void* userPtr);
    void submitWork(TangleWork work, void* userPtr, TangleGroup* group);
    void submitMultiple(TangleWork work, void* userBuffer, int stride,
                        TangleGroup* group, unsigned int jobCount,
                        TangleGroup* submitGroup);
    void submitMultipleSync(TangleWork work, void* userBuffer, int stride,
                            TangleGroup* group, unsigned int jobCount);

    void blockThreads();
    void unblockThreads();
    void finishWork();
  }
}

#endif

#endif
