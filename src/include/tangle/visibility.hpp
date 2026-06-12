#ifndef TANGLEVISIBILITY
#define TANGLEVISIBILITY

#if __has_attribute(visibility)
  #define TANGLE_EXPOSED __attribute__((visibility("default")))
  #define TANGLE_INTERNAL __attribute__((visibility("hidden")))
#elif __has_cpp_attribute(gnu::visibility)
  #define TANGLE_EXPOSED [[gnu::visibility("default")]]
  #define TANGLE_INTERNAL [[gnu::visibility("hidden")]]
#else
  #define TANGLE_EXPOSED
  #define TANGLE_INTERNAL
  #error "Can't control symbol visibility, expect problems"
#endif

#endif
