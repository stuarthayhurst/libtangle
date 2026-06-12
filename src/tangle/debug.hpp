#ifndef INTERNALDEBUGHEADER
#define INTERNALDEBUGHEADER

/*
 - Output sent to tangleInternalDebug will disappear unless TANGLE_DEBUG is defined
 - Expressions won't even be evaluated, logging to debug is free in production
*/
#ifdef TANGLE_DEBUG
  #include "logging.hpp"
  //NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern tangle::utils::OutputHelper tangleInternalDebug;
#else
  //NOLINTNEXTLINE(misc-include-cleaner)
  #include <iostream>
  #define tangleInternalDebug \
  if constexpr (false) std::cout
#endif

#endif
