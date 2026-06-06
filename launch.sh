#!/bin/bash

#Run the tests using the locally built library

#Use --loop to repeatedly run the tests
#Use --valgrind to run the selected target through valgrind (ignored for loop mode)
#  - Set USE_VALGRIND_PATH to use a specific path to the valgrind binary
#Any unrecognised arguments will be passed to the target

buildDir="build"
NEW_LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$buildDir"

target="$buildDir/threadTest"
targetArgs=()

loopRequested="false"
valgrindRequested="false"

#Match arguments, filtering out arguments handled by launch.sh
for arg in "$@"; do
  case "$arg" in
    --loop)
      loopRequested="true"
      ;;
    --valgrind)
      valgrindRequested="true"
      ;;
    *)
      targetArgs+=("$arg")
      ;;
  esac
done

#Check that the selected target exists
if [[ ! -f "$target" ]]; then
  echo "'$target' doesn't exist, did you forget to build it?" > /dev/stderr
  exit 1
fi

#Pick a path to valgrind, if requested
USE_VALGRIND_PATH=""
if [[ "$valgrindRequested" == "true" ]]; then
  if [[ "$VALGRIND" != "" ]]; then
    USE_VALGRIND_PATH="$VALGRIND"
  else
    USE_VALGRIND_PATH="valgrind"
  fi
fi

#Run the target in a loop or just once
if [[ "$loopRequested" == "true" ]]; then
  while true; do
    if ! LD_LIBRARY_PATH="$NEW_LD_LIBRARY_PATH" "$target" "${targetArgs[@]}"; then
      exit 1
    fi
  done
else
  #Use valgrind if configured to
  if [[ "$USE_VALGRIND_PATH" == "" ]]; then
    LD_LIBRARY_PATH="$NEW_LD_LIBRARY_PATH" "$target" "${targetArgs[@]}"
  else
    LD_LIBRARY_PATH="$NEW_LD_LIBRARY_PATH" "$USE_VALGRIND_PATH" --leak-check=full "$target" "${targetArgs[@]}"
  fi
fi
