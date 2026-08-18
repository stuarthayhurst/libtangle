#ifndef TANGLEGROUP
#define TANGLEGROUP

#include <climits>
#include <semaphore>

#include "visibility.hpp"

/*
 - Initialise using {0}, for example 'TangleGroup group{0};'
 - Multiple submit calls can share the same group
 - A group can be reused without reinitialising it
*/
using TangleGroup = std::counting_semaphore<INT_MAX>;

namespace TANGLE_EXPOSED tangle {
  namespace thread {
    void waitGroupComplete(TangleGroup* group, unsigned int jobCount);
    bool isSingleWorkComplete(TangleGroup* group);
    unsigned int getRemainingWork(TangleGroup* group, unsigned int jobCount);
  }
}

#endif
