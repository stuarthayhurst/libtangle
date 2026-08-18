#ifndef TANGLETHREAD
#define TANGLETHREAD

#include <climits>
#include <semaphore>

#include "visibility.hpp"

//Functions of this type must not block conditionally on other jobs
using TangleWork = void (*)(void* userPtr);

/*
 - Initialise using {0}, for example 'TangleGroup group{0};'
 - Multiple submit calls can share the same group
 - A group can be reused without reinitialising it
*/
using TangleGroup = std::counting_semaphore<INT_MAX>;

namespace TANGLE_EXPOSED tangle {
  namespace thread {
    unsigned int getHardwareThreadCount();
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

    void waitGroupComplete(TangleGroup* group, unsigned int jobCount);
    bool isSingleWorkComplete(TangleGroup* group);
    unsigned int getRemainingWork(TangleGroup* group, unsigned int jobCount);

    void blockThreads(void* threadPool);
    void unblockThreads(void* threadPool);
    void finishWork(void* threadPool);
  }
}

#endif
