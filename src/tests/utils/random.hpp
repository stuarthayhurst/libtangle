#ifndef RANDOMUTILS
#define RANDOMUTILS

#include <random>

namespace tests {
  namespace utils {
    namespace internal {
      /*
       - Defined inside random.cpp, exposed for usage here
       - Don't use this directly, use the helpers instead
      */

      //NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
      extern thread_local std::random_device r;
      extern thread_local std::default_random_engine engine;
      //NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
    }

    /*
     - Return a random value of a given type from the closed interval [lower, upper]
       - Negative numbers are supported
    */
    template <typename T> requires std::is_integral_v<T>
    inline T random(T lower, T upper) {
      return std::uniform_int_distribution<T>{lower, upper}(internal::engine);
    }

    //Return a random value of a given type from the closed interval [0, upper]
    template <typename T> requires std::is_integral_v<T>
    inline T random(T upper) {
      return random(T(0.0), upper);
    }
  }
}

#endif
