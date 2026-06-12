#include <random>

#include "random.hpp"

namespace tests {
  namespace utils {
    namespace internal {
      //Create a random engine and seed it
      //NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
      thread_local std::random_device r;
      thread_local std::default_random_engine engine(r());
      //NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
    }
  }
}
