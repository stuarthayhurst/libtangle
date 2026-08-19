#ifndef TANGLETHREAD
#define TANGLETHREAD

#include "group.hpp"
#include "visibility.hpp"

//Functions of this type must not block conditionally on other jobs
using TangleWork = void (*)(void* userPtr);

namespace TANGLE_EXPOSED tangle {
  namespace thread {
    unsigned int getHardwareThreadCount();
    unsigned int getExpectedThreadPoolSize(unsigned int threadCount);
    unsigned int getThreadPoolSize(void* threadPool);

    void* createThreadPoolInstance(unsigned int threadCount);
    void destroyThreadPool(void* threadPool);

    void submitWork(TangleWork work, void* userPtr, void* threadPool);
    void submitWork(TangleWork work, void* userPtr, TangleGroup* group, void* threadPool);
    void submitMultiple(TangleWork work, void* userBuffer, int stride,
                        TangleGroup* group, unsigned int jobCount,
                        TangleGroup* submitGroup, void* threadPool);
    void submitMultipleSync(TangleWork work, void* userBuffer, int stride,
                            TangleGroup* group, unsigned int jobCount, void* threadPool);

    void blockThreads(void* threadPool);
    void unblockThreads(void* threadPool);
    void finishWork(void* threadPool);
  }
}

#endif
