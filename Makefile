SHELL := bash -O globstar
TIDY ?= clang-tidy

BUILD_DIR ?= build
OBJECT_DIR := $(BUILD_DIR)/objects
PREFIX_DIR ?= /usr/local
INSTALL_DIR ?= $(PREFIX_DIR)/lib
HEADER_DIR ?= $(PREFIX_DIR)/include
PKG_CONF_DIR ?= $(INSTALL_DIR)/pkgconfig
LIBRARY_VERSION := $(shell pkg-config --modversion data/tangle.pc)
LIBRARY_NAME := libtangle.so.$(LIBRARY_VERSION)

TANGLE_OBJECTS_SOURCE := $(shell ls ./src/tangle/**/*.cpp)
TANGLE_HEADERS_SOURCE := $(shell ls ./src/tangle/**/*.hpp)
TANGLE_INCLUDE_HEADERS_SOURCE += $(shell ls ./src/include/tangle/**/*.hpp)

TEST_OBJECTS_SOURCE := $(shell ls ./src/tests/**/*.cpp)
TEST_HEADERS_SOURCE := $(shell ls ./src/tests/**/*.hpp)

ROOT_OBJECTS_SOURCE := $(shell ls ./src/*.cpp)

LINT_OBJECTS_SOURCE := $(ROOT_OBJECTS_SOURCE) $(TANGLE_OBJECTS_SOURCE)
LINT_HEADERS_SOURCE := $(ROOT_HEADERS_SOURCE) $(TANGLE_INCLUDE_HEADERS_SOURCE)

DEBUG_LINT_STRING := linted
ifeq ($(DEBUG),true)
  DEBUG_LINT_STRING := debug.linted
endif

LINT_FILES := $(subst ./src,$(OBJECT_DIR),$(subst .hpp,.hpp.$(DEBUG_LINT_STRING),$(LINT_HEADERS_SOURCE)))
LINT_FILES += $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.cpp.$(DEBUG_LINT_STRING),$(LINT_OBJECTS_SOURCE)))
TEST_LINT_FILES := $(subst ./src,$(OBJECT_DIR),$(subst .hpp,.hpp.$(DEBUG_LINT_STRING),$(TEST_HEADERS_SOURCE)))
TEST_LINT_FILES += $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.cpp.$(DEBUG_LINT_STRING),$(TEST_OBJECTS_SOURCE)))

TANGLE_OBJECTS := $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(TANGLE_OBJECTS_SOURCE)))
TEST_OBJECTS := $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(TEST_OBJECTS_SOURCE)))
ROOT_OBJECTS := $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(ROOT_OBJECTS_SOURCE)))

#Global arguments
CXXFLAGS += -Wall -Wextra -Werror -Wpedantic -std=c++23
CXXFLAGS += -flto=auto

ifndef ARCH
  CXXFLAGS += -march=native
else
  ifneq ($(ARCH),unset)
    CXXFLAGS += -march=$(ARCH)
  endif
endif

ifeq ($(USE_LLVM_CPP),true)
  CXXFLAGS += -stdlib=libc++
  LDFLAGS += -stdlib=libc++
endif


ifeq ($(DEBUG),true)
  CXXFLAGS += -DTANGLE_DEBUG -g -Og -fno-omit-frame-pointer

  #Enable ASan and UBSan by default in debug mode if nothing incompatible is enabled
  ifeq (,$(filter true,$(CHECK_THREADS) $(CHECK_TYPES) $(CHECK_MEMORY)))
    ifndef CHECK_ADDRESS
      CHECK_ADDRESS := true
    endif
    ifndef CHECK_UNDEFINED
      CHECK_UNDEFINED := true
    endif
  endif
else
  CXXFLAGS += -O3
endif

ifneq ($(VALGRIND_SAFE),true)
  ifeq ($(CHECK_ADDRESS),true)
    CXXFLAGS += -fsanitize=address
  endif

  ifeq ($(CHECK_UNDEFINED),true)
    CXXFLAGS += -fsanitize=undefined
  endif

  ifeq ($(CHECK_THREADS),true)
    CXXFLAGS += -fsanitize=thread
  endif

  ifeq ($(CHECK_TYPES),true)
    CXXFLAGS += -fsanitize=type
  endif

  ifeq ($(CHECK_MEMORY),true)
    CXXFLAGS += -fsanitize=memory
  endif

  ifeq ($(CHECK_LEAKS),true)
    LDFLAGS += -fsanitize=leak
  endif
endif

#Fetch library dependencies and flags from tangle.pc
LDFLAGS_PRIVATE := $(shell sed -ne 's/^.*Libs.private: //p' data/tangle.pc)
CFLAGS_PRIVATE := $(shell sed -ne 's/^.*Cflags.private: //p' data/tangle.pc)

#Library arguments
LIBRARY_CXXFLAGS := $(CXXFLAGS) -fpic $(CFLAGS_PRIVATE)
LIBRARY_LDFLAGS := $(LDFLAGS) "-Wl,-soname,$(LIBRARY_NAME)" $(LDFLAGS_PRIVATE)

#Client arguments
ifneq ($(USE_SYSTEM),true)
  PROJECT_ROOT := $(dir $(realpath $(firstword $(MAKEFILE_LIST))))
  PKG_CONF_ARGS := "--define-variable=tanglelibdir=$(BUILD_DIR)" \
                   "--define-variable=tangleincludedir=$(PROJECT_ROOT)/src/include" \
                   "--with-path=$(PROJECT_ROOT)/data"
  PKG_CONF_FILE := data/tangle.pc
else
  PKG_CONF_FILE := tangle
endif

CLIENT_CXXFLAGS := $(CXXFLAGS) $(shell pkg-config $(PKG_CONF_ARGS) --cflags $(PKG_CONF_FILE))
CLIENT_LDFLAGS := $(LDFLAGS) $(shell pkg-config $(PKG_CONF_ARGS) --libs $(PKG_CONF_FILE))

#Recipe-specific client arguments
THREADTEST_EXTRA_LDFLAGS := -latomic

#Helper to run the compiler or extract the command
EXTRACT_SCRIPT := python3 extract-command.py
EXTRACT := @function inline() { if [[ "$(DUMMY)" != "true" ]]; then echo "$(CXX) $$@"; $(CXX) $$@; else $(EXTRACT_SCRIPT) "$(BUILD_DIR)" $(CXX) $$@; fi }; inline

# --------------------------------
# Client build recipes
# --------------------------------

$(BUILD_DIR)/threadTest: $(BUILD_DIR)/$(LIBRARY_NAME) $(OBJECT_DIR)/threadTest.o $(TEST_OBJECTS)
	@mkdir -p "$(BUILD_DIR)"
	$(CXX) -o "$(BUILD_DIR)/threadTest" $(OBJECT_DIR)/threadTest.o $(TEST_OBJECTS) $(CLIENT_CXXFLAGS) $(CLIENT_LDFLAGS) $(THREADTEST_EXTRA_LDFLAGS)

#Recipe dependencies need to be mirrored in the corresponding lint targets
$(OBJECT_DIR)/tests/%.o: ./src/tests/%.cpp $(TEST_HEADERS_SOURCE) $(TANGLE_INCLUDE_HEADERS_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(EXTRACT) "$<" -c $(CLIENT_CXXFLAGS) -o "$@"
$(OBJECT_DIR)/%.o: ./src/%.cpp $(TEST_HEADERS_SOURCE) $(TANGLE_INCLUDE_HEADERS_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(EXTRACT) "$<" -c $(CLIENT_CXXFLAGS) -o "$@"

# --------------------------------
# Library build recipes
# --------------------------------

$(BUILD_DIR)/libtangle.so: $(TANGLE_OBJECTS)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) -shared -o "$@" $(TANGLE_OBJECTS) $(LIBRARY_CXXFLAGS) $(LIBRARY_LDFLAGS)
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$@"; \
	fi
$(BUILD_DIR)/$(LIBRARY_NAME): $(BUILD_DIR)/libtangle.so
	@rm -fv "$(BUILD_DIR)/$(LIBRARY_NAME)"
	@ln -sv "libtangle.so" "$(BUILD_DIR)/$(LIBRARY_NAME)"

#Recipe dependencies need to be mirrored in the corresponding lint targets
$(OBJECT_DIR)/tangle/%.o: ./src/tangle/%.cpp $(TANGLE_HEADERS_SOURCE) $(TANGLE_INCLUDE_HEADERS_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(EXTRACT) "$<" -c $(LIBRARY_CXXFLAGS) -o "$@"


# --------------------------------
# Shared linting recipes
# --------------------------------

$(BUILD_DIR)/compile_commands.json:
	@DUMMY="true" $(MAKE) $(TANGLE_OBJECTS) $(ROOT_OBJECTS) $(TEST_OBJECTS)

#Recipes should only differ in dependencies
#include/% recipes are required, since headers can be linted individually
$(OBJECT_DIR)/include/%.$(DEBUG_LINT_STRING): ./src/include/% .clang-tidy $(TANGLE_INCLUDE_HEADERS_SOURCE)
	$(TIDY) --quiet -p "$(BUILD_DIR)" "$<"
	@mkdir -p "$$(dirname $@)"
	@touch "$@"
$(OBJECT_DIR)/tests/%.$(DEBUG_LINT_STRING): ./src/tests/% .clang-tidy $(TEST_HEADERS_SOURCE) $(TANGLE_INCLUDE_HEADERS_SOURCE)
	$(TIDY) --quiet -p "$(BUILD_DIR)" "$<"
	@mkdir -p "$$(dirname $@)"
	@touch "$@"
$(OBJECT_DIR)/%.$(DEBUG_LINT_STRING): ./src/% .clang-tidy $(TEST_HEADERS_SOURCE) $(TANGLE_INCLUDE_HEADERS_SOURCE)
	$(TIDY) --quiet -p "$(BUILD_DIR)" "$<"
	@mkdir -p "$$(dirname $@)"
	@touch "$@"

$(OBJECT_DIR)/tangle/%.$(DEBUG_LINT_STRING): ./src/tangle/% .clang-tidy $(TANGLE_HEADERS_SOURCE) $(TANGLE_INCLUDE_HEADERS_SOURCE)
	$(TIDY) --quiet -p "$(BUILD_DIR)" "$<"
	@mkdir -p "$$(dirname $@)"
	@touch "$@"


.PHONY: build tests all threads debug debug-all library headers install uninstall lint_compile_commands run_lint run_lint_tests lint lint_tests lint_all clean


# --------------------------------
# Client phony recipes
# --------------------------------

build: library
tests: threads
all: library tests
demo: $(BUILD_DIR)/demo
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$<"; \
	fi
threads: $(BUILD_DIR)/threadTest
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$<"; \
	fi
debug:
	@DEBUG="true" $(MAKE) --no-print-directory build
debug-all:
	@DEBUG="true" $(MAKE) --no-print-directory all


# --------------------------------
# Library phony recipes
# --------------------------------

library: $(BUILD_DIR)/$(LIBRARY_NAME)


# --------------------------------
# Installer phony recipes
# --------------------------------

headers:
	@rm -rf "$(HEADER_DIR)/tangle"
	@mkdir -p "$(HEADER_DIR)"
	@cp -rv "src/include/tangle" "$(HEADER_DIR)/tangle"
	@mkdir -p "$(PKG_CONF_DIR)"
	install "data/tangle.pc" "$(PKG_CONF_DIR)/tangle.pc"
	sed -e "s|prefix=/usr/local|prefix=$(PREFIX_DIR)|" "data/tangle.pc" > "$(PKG_CONF_DIR)/tangle.pc"
install:
	@mkdir -p "$(INSTALL_DIR)"
	install "$(BUILD_DIR)/libtangle.so" "$(INSTALL_DIR)/$(LIBRARY_NAME)"
	@ln -sfv "$(LIBRARY_NAME)" "$(INSTALL_DIR)/libtangle.so"
	ldconfig "$(INSTALL_DIR)"
uninstall:
	@rm -fv "$(INSTALL_DIR)/libtangle.so"*
	@rm -fv "$(PKG_CONF_DIR)/tangle.pc"
	@if [[ -d "$(HEADER_DIR)/tangle" ]]; then rm -rv "$(HEADER_DIR)/tangle"; fi
	ldconfig


# --------------------------------
# Linting phony recipes
# --------------------------------

lint_compile_commands:
	@$(MAKE) -B --no-print-directory $(BUILD_DIR)/compile_commands.json
run_lint: $(LINT_FILES)
run_lint_tests: $(TEST_LINT_FILES)
lint: lint_compile_commands
	@$(MAKE) --no-print-directory run_lint
lint_tests: lint_compile_commands
	@$(MAKE) --no-print-directory run_lint_tests
lint_all: lint lint_tests

# --------------------------------
# Utility / support phony recipes
# --------------------------------

clean:
	@rm -rfv "$(BUILD_DIR)"
