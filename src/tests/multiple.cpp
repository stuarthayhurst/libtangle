#include <cstdlib>

#include <tangle/tangle.hpp>

#include "tests.hpp"

#include "common.hpp"
#include "utils/utils.hpp"

namespace tests {
  bool testSubmitMultiple(unsigned int jobCount) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);
    TangleGroup group{0};

    //Submit fast 'jobs'
    tests::common::resetTimers(timers);
    tangle::thread::submitMultiple(tests::common::shortTask, &values[0], sizeof(values[0]),
                                   &group, jobCount, NO_GROUP);
    tests::common::finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&group, jobCount);
    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    const bool passed = tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    tests::common::destroyThreadPool();
    tests::common::destroyTimers(timers);
    return passed;
  }

  bool testSubmitMultipleMultiple(unsigned int jobCount) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount * 4);
    TangleGroup group{0};

    //Submit fast 'jobs'
    tests::common::resetTimers(timers);
    for (int i = 0; i < 4; i++) {
      tangle::thread::submitMultiple(tests::common::shortTask, &values[(std::size_t)jobCount * i],
                                     sizeof(values[0]), &group, jobCount, nullptr);
    }
    tests::common::finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&group, jobCount * 4);
    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    const bool passed = tests::common::verifyWork(jobCount * 4, values);

    tests::common::destroyValues(values);
    tests::common::destroyThreadPool();
    tests::common::destroyTimers(timers);
    return passed;
  }

  bool testSubmitMultipleSyncSubmit(unsigned int jobCount) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);
    TangleGroup group{0};

    //Submit fast 'jobs'
    tests::common::resetTimers(timers);
    tangle::thread::submitMultipleSync(tests::common::shortTask, &values[0], sizeof(values[0]),
                                       &group, jobCount);
    tests::common::finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&group, jobCount);
    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    const bool passed = tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    tests::common::destroyThreadPool();
    tests::common::destroyTimers(timers);
    return passed;
  }

  bool testSubmitMultipleNoSync(unsigned int jobCount) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);
    TangleGroup submitGroup{0};

    //Submit fast 'jobs'
    tests::common::resetTimers(timers);
    tangle::thread::submitMultiple(tests::common::shortTask, &values[0], sizeof(values[0]),
                                   nullptr, jobCount, &submitGroup);
    tests::common::finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&submitGroup, 1);
    tests::common::destroyThreadPool();
    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    const bool passed = tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    tests::common::destroyTimers(timers);
    return passed;
  }
}
