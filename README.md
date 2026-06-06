## libtangle
  - C++ library to provide a thread pool and synchronised output helpers
  - Originally written as part of [Ammonite](https://github.com/stuarthayhurst/ammonite-engine), extracted here

## Building + installing libtangle:
  - `make library`
  - `sudo make install`
  - Optionally, run `sudo make headers` to install the headers

comment on header installation as optional for dev

## Build system:
  - ### Targets:
    - `build` and `library` support `-j[CORE COUNT]`
    - `make build` - Builds the library and tests
    - `make library` - Builds `build/libtangle.so`
    - `make install` - Installs `libtangle.so` to the system
    - `make headers` - Installs libtangle headers and `tangle.pc` to the system
    - `make uninstall` - Removes installed library and headers
      - Custom install locations must be specified the same way as they were installed
    - `make clean` - Cleans the build area (`build/`)
  - All targets and optional flags are documented [here](docs/CONTRIBUTING.md#build-system)
  - Set the environment variable `PREFIX_DIR` to configure the base install path
    - The `install`, `headers` and `uninstall` targets have additional path options

## Dependencies:
  - Package names are correct for Debian, other distros may vary
  - `make`
  - `pkgconf`
  - `coreutils`
  - `python3`
  - `g++ (16+)` **OR** `clang++ (19+)`
    - If using clang++, use `CXX="clang++" make [TARGET]`
      - `USE_LLVM_CPP=true` might be useful for systems without (a new enough) GCC
    - When swapping between different compilers, run `make clean`
  - ### Linting:
    - `clang-tidy (22+)`

## Licence:
  - This project is available under the terms of the MIT License
    - These terms can be found in `LICENCE.txt`
