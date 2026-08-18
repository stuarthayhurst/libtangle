#ifndef INTERNALGROUP
#define INTERNALGROUP

#include "visibility.hpp"

//Include public interface
#include "../include/tangle/group.hpp" // IWYU pragma: export

namespace TANGLE_INTERNAL tangle {
  namespace thread {
    namespace internal {
      void waitGroupComplete(TangleGroup* group, unsigned int jobCount);
    }
  }
}

#endif
