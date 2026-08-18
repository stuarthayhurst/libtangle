#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

#include <tangle/tangle.hpp>

#include "tests.hpp"

#include "common.hpp"
#include "utils/utils.hpp"

namespace tests {
  bool testOutputHelpers(unsigned int jobCount) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    TangleGroup group{0};

    //Prepare logging jobs
    unsigned int* const values = tests::common::createValues(jobCount);
    const unsigned int outputCount = 1000;
    common::LoggingData loggingData{.outputCount = outputCount, .index = 0, .values = values};
    for (unsigned int i = 0; i < jobCount; i++) {
      values[i] = i;
    }

    //Submit logging jobs
    tests::common::resetTimers(timers);
    tangle::thread::submitMultiple(tests::common::loggingTask, &loggingData, 0,
                                   &group, jobCount, NO_GROUP);
    tests::common::finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&group, jobCount);
    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    bool passed = tests::common::verifyWork(jobCount, values);

    //Verify output blocks
    std::string threadOutput;
    std::unordered_set<unsigned int> foundValues;
    while (std::getline(tests::common::outputCapture, threadOutput)) {
      std::stringstream lineStream(threadOutput);
      std::string component;

      //Extract and verify prefix
      std::getline(lineStream, component, ' ');
      if (component != std::string("PREFIX:")) {
        tangle::utils::error << "Failed to verify output prefix" << std::endl;
        tangle::utils::error << "Expected: PREFIX:" << std::endl;
        tangle::utils::error << "Got:" << component << std::endl;
        passed = false;
      }

      //Extract the value used for the data
      std::string value;
      std::getline(lineStream, value, ' ');
      foundValues.insert(stoi(value));

      //Generate expected output block
      std::getline(lineStream, component);
      std::string expected;
      for (unsigned int i = 0; i < outputCount; i++) {
        expected.append(value);
      }

      //Verify output block
      if (component != expected) {
        tangle::utils::error << "Failed to verify output block" << std::endl;
        tangle::utils::error << "Expected:" << expected << std::endl;
        tangle::utils::error << "Got:" << component << std::endl;
        passed = false;
      }
    }

    //Verify all numbers were seen
    for (unsigned int i = 0; i < jobCount; i++) {
      if (!foundValues.contains(i)) {
        tangle::utils::error << "Failed to verify value '" << i << "'" << std::endl;
        passed = false;
      }
    }

    tests::common::destroyValues(values);
    tests::common::destroyThreadPool();
    tests::common::destroyTimers(timers);
    return passed;
  }
}
