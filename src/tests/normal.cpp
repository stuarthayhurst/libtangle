#include <atomic>
#include <cstdlib>
#include <iostream>

#include <tangle/tangle.hpp>

#include "tests.hpp"

#include "common.hpp"
#include "utils/utils.hpp"

namespace tests {
  bool testCreateSubmitWaitDestroy(unsigned int jobCount) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);
    TangleGroup group{0};

    //Submit fast 'jobs'
    tests::common::resetTimers(timers);
    tests::common::submitShortSyncJobs(jobCount, values, &group);
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

  bool testCreateSubmitBlockUnblockDestroy(unsigned int jobCount) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);

    //Submit fast 'jobs'
    tests::common::resetTimers(timers);
    tests::common::submitShortJobs(jobCount, values);
    tests::common::finishSubmitTimer(timers);

    //Finish work
    tangle::thread::finishWork();
    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    const bool passed = tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    tests::common::destroyThreadPool();
    tests::common::destroyTimers(timers);
    return passed;
  }

  bool testCreateSubmitDestroy(unsigned int jobCount) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);

    //Submit fast 'jobs'
    tests::common::resetTimers(timers);
    tests::common::submitShortJobs(jobCount, values);
    tests::common::finishSubmitTimer(timers);

    //Finish work
    tests::common::destroyThreadPool();
    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    const bool passed = tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    tests::common::destroyTimers(timers);
    return passed;
  }

  bool testCreateBlockSubmitUnblockWaitDestroy(unsigned int jobCount) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);
    TangleGroup group{0};

    tangle::thread::blockThreads();

    //Submit fast 'jobs'
    tests::common::resetTimers(timers);
    tests::common::submitShortSyncJobs(jobCount, values, &group);
    tests::common::finishSubmitTimer(timers);

    //Finish work
    tangle::thread::unblockThreads();
    tangle::thread::waitGroupComplete(&group, jobCount);
    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    const bool passed = tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    tests::common::destroyThreadPool();
    tests::common::destroyTimers(timers);
    return passed;
  }

  bool testQueueLimits(unsigned int jobCount) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    jobCount *= 4;
    unsigned int* values = tests::common::createValues(jobCount);
    TangleGroup group{0};

    //Submit fast 'jobs'
    tests::common::resetTimers(timers);
    tests::common::submitShortSyncJobs(jobCount, values, &group);
    tests::common::finishSubmitTimer(timers);

    //Clean up after the first batch
    tangle::thread::waitGroupComplete(&group, jobCount);
    bool passed = tests::common::verifyWork(jobCount, values);
    tests::common::destroyValues(values);

    //Submit second batch
    values = tests::common::createValues(jobCount);
    tests::common::resumeSubmitTimer(timers);
    tests::common::submitShortSyncJobs(jobCount, values, &group);
    tests::common::finishSubmitTimer(timers);

    //Clean up after the second batch
    tangle::thread::waitGroupComplete(&group, jobCount);
    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    passed &= tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    tests::common::destroyThreadPool();
    tests::common::destroyTimers(timers);
    return passed;
  }

  bool testNestedJobs(unsigned int fullJobCount) {
    const unsigned int jobCount = fullJobCount / 2;
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);
    TangleGroup group{0};

    //Submit nested 'jobs'
    tests::common::resetTimers(timers);
    tests::common::ResubmitData* const data = new tests::common::ResubmitData[jobCount]{};
    for (unsigned int i = 0; i < jobCount; i++) {
      data[i].writePtr = &values[i];
      data[i].syncPtr = &group;
      tangle::thread::submitWork(tests::common::resubmitTask, &data[i], (TangleGroup*)nullptr);
    }
    tests::common::finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&group, jobCount);
    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    const bool passed = tests::common::verifyWork(jobCount, values);

    delete [] data;
    tests::common::destroyValues(values);
    tests::common::destroyThreadPool();
    tests::common::destroyTimers(timers);
    return passed;
  }

  bool testChainJobs(unsigned int jobCount) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    const unsigned int poolSize = tangle::thread::getThreadPoolSize();
    const unsigned int totalJobCount = jobCount * poolSize;
    unsigned int* const values = tests::common::createValues(totalJobCount);
    TangleGroup sync{0};

    tests::common::ChainData* const userDataArray = new tests::common::ChainData[poolSize];
    for (std::size_t i = 0; i < poolSize; i++) {
      userDataArray[i].totalSubmitted = 1;
      userDataArray[i].targetSubmitted = jobCount;
      userDataArray[i].work = tests::common::chainTask;
      userDataArray[i].values = values + (i * jobCount);
      userDataArray[i].syncPtr = &sync;
    }

    //Submit chain 'jobs'
    tests::common::resetTimers(timers);
    tangle::thread::submitMultiple(tests::common::chainTask, userDataArray,
                                   sizeof(tests::common::ChainData), &sync, poolSize, nullptr);
    tests::common::finishSubmitTimer(timers);

    tangle::thread::waitGroupComplete(&sync, totalJobCount);
    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    const bool passed = tests::common::verifyWork(totalJobCount, values);

    delete [] userDataArray;
    tests::common::destroyValues(values);
    tests::common::destroyThreadPool();
    tests::common::destroyTimers(timers);
    return passed;
  }

  bool testSingleSyncHelper(unsigned int jobCount) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    TangleGroup group{0};

    //Test single work complete check before work is submitted
    if (tangle::thread::isSingleWorkComplete(&group)) {
      tangle::utils::error << "Single work incorrectly reported as complete" << std::endl;
      return false;
    }

    //Prepare flags to control job flow
    std::atomic_flag* const flags = new std::atomic_flag[jobCount]{ATOMIC_FLAG_INIT};

    //Submit controlled blocking jobs
    tests::common::resetTimers(timers);
    for (unsigned int i = 0; i < jobCount; i++) {
      tangle::thread::submitWork(tests::common::blockingTask, &flags[i], &group);
    }
    tests::common::finishSubmitTimer(timers);

    //Process the work
    for (unsigned int i = 0; i < jobCount; i++) {
      //Check not jobs have unexpectedly finished
      if (tangle::thread::isSingleWorkComplete(&group)) {
        tangle::utils::error << "Single work incorrectly reported as complete" << std::endl;
        delete [] flags;
        return false;
      }

      //Allow a job to progress
      flags[i].test_and_set();
      flags[i].notify_all();

      //Spin until the job finishes
      while (!tangle::thread::isSingleWorkComplete(&group)) {};
    }

    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);

    delete [] flags;
    tests::common::destroyThreadPool();
    tests::common::destroyTimers(timers);
    return true;
  }

  bool testMultipleSyncHelper(unsigned int jobCount) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);
    TangleGroup group{0};

    //Test multiple work complete check before work is submitted
    if (tangle::thread::getRemainingWork(&group, jobCount) != jobCount) {
      tangle::utils::error << "Work incorrectly reported as complete" << std::endl;
      return false;
    }

    //Submit 'fast' jobs
    tests::common::resetTimers(timers);
    tangle::thread::submitMultiple(tests::common::shortTask, &values[0], sizeof(values[0]),
                                   &group, jobCount, nullptr);
    tests::common::finishSubmitTimer(timers);

    //Wait for the jobs to complete
    unsigned int remainingJobs = jobCount;
    while (remainingJobs != 0) {
      remainingJobs = tangle::thread::getRemainingWork(&group, remainingJobs);
    }

    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    const bool passed = tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    tests::common::destroyThreadPool();
    tests::common::destroyTimers(timers);
    return passed;
  }
}
