#ifdef TANGLE_DEBUG

#include <iostream>

#include "debug.hpp"

#include "logging.hpp"

//NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables, cppcoreguidelines-interfaces-global-init)
tangle::utils::OutputHelper tangleInternalDebug(std::cout, "DEBUG: ",
                                                tangle::utils::colour::magenta);
#endif
