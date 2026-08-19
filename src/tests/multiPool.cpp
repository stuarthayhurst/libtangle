#include <tangle/tangle.hpp>

#include "tests.hpp"

#include "common.hpp"
#include "utils/utils.hpp"

namespace tests {
  bool testPingPongPools(unsigned int jobCount) {
    //Create the thread pool pair
    tests::utils::Timer* const timers = tests::common::createTimers();
    const unsigned int threadCount = tangle::thread::getHardwareThreadCount();

    void* const threadPoolA = tests::common::createThreadPoolInstance(threadCount);
    if (threadPoolA == nullptr) {
      tests::common::destroyTimers(timers);
      return false;
    }

    void* const threadPoolB = tests::common::createThreadPoolInstance(threadCount);
    if (threadPoolB == nullptr) {
      tests::common::destroyThreadPool(threadPoolA);
      tests::common::destroyTimers(timers);
      return false;
    }

    unsigned int* const values = tests::common::createValues(jobCount);
    TangleGroup group{0};

    //Prepare the data half executed by threadPoolA, pointing at threadPoolB
    tests::common::PingPongData dataA = {
      .syncPtr = &group, .otherPool = threadPoolB, .otherData = nullptr,
      .totalSubmitted = 0, .targetSubmitted = jobCount / 2, .values = values
    };

    //Prepare the data half executed by threadPoolB, pointing at threadPoolA
    tests::common::PingPongData dataB = {
      .syncPtr = &group, .otherPool = threadPoolA, .otherData = nullptr,
      .totalSubmitted = 1, .targetSubmitted = jobCount / 2, .values = values + (jobCount / 2)
    };

    //Point the data halves at each other
    dataA.otherData = &dataB;
    dataB.otherData = &dataA;

    //Submit ping pong jobs
    tests::common::resetTimers(timers);
    tangle::thread::submitWork(common::pingPongTask, &dataA, &group, threadPoolA);
    tests::common::finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&group, jobCount);
    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    const bool passed = tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    tests::common::destroyThreadPool(threadPoolA);
    tests::common::destroyThreadPool(threadPoolB);
    tests::common::destroyTimers(timers);
    return passed;
  }
}
