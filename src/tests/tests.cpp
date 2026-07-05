#include <atomic>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

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
      tangle::thread::submitWork(tests::common::resubmitTask, &data[i], nullptr);
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
                                   &group, jobCount, nullptr);
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

  bool testRandomWorkloads(unsigned int batchSize) {
    tests::utils::Timer* const timers = tests::common::createTimers();
    if (!tests::common::createThreadPool(0)) {
      tests::common::destroyTimers(timers);
      return false;
    }
    const unsigned int testCount = 15;
    const unsigned int totalJobCount = batchSize * testCount;
    unsigned int* const values = tests::common::createValues(totalJobCount);

    struct BatchInfo {
      TangleGroup* group;
      unsigned int waitCount = 0;
    };

    tests::common::resetTimers(timers);
    std::vector<BatchInfo> batchInfoVector;
    std::vector<tests::common::ChainData*> chainDataVector;
    for (std::size_t testIndex = 0; testIndex < testCount; testIndex++) {
      const unsigned int jobTypeCount = 7;
      unsigned int* const offsetValues = values + (testIndex * batchSize);

      switch (tests::utils::random<unsigned int>(jobTypeCount - 1)) {
      case 0:
        tangle::utils::normal << "  " << testIndex \
                              << ": Testing regular submit, with explicit sync" \
                              << std::endl;
        {
          BatchInfo& batchInfo = batchInfoVector.emplace_back();
          batchInfo.waitCount = batchSize;
          batchInfo.group = new TangleGroup{0};

          tests::common::submitShortSyncJobs(batchSize, offsetValues, batchInfo.group);
          break;
        }
      case 1:
        tangle::utils::normal << "  " << testIndex \
                              << ": Testing regular submit, without explicit sync" \
                              << std::endl;
        tests::common::submitShortSyncJobs(batchSize, offsetValues, nullptr);
        break;
      case 2:
        tangle::utils::normal << "  " << testIndex \
                              << ": Testing submit multiple, sync on jobs" \
                              << std::endl;
        {
          BatchInfo& batchInfo = batchInfoVector.emplace_back();
          batchInfo.waitCount = batchSize;
          batchInfo.group = new TangleGroup{0};

          tangle::thread::submitMultiple(tests::common::shortTask, offsetValues, sizeof(values[0]),
                                         batchInfo.group, batchSize, nullptr);
          break;
        }
      case 3:
        tangle::utils::normal << "  " << testIndex \
                              << ": Testing submit multiple, sync on submit" \
                              << std::endl;
        {
          BatchInfo& batchInfo = batchInfoVector.emplace_back();
          batchInfo.waitCount = 1;
          batchInfo.group = new TangleGroup{0};

          tangle::thread::submitMultiple(tests::common::shortTask, offsetValues, sizeof(values[0]),
                                         nullptr, batchSize, batchInfo.group);
          break;
        }
      case 4:
        tangle::utils::normal << "  " << testIndex \
                              << ": Testing submit multiple, synchronous submit" \
                              << std::endl;
        {
          BatchInfo& batchInfo = batchInfoVector.emplace_back();
          batchInfo.waitCount = batchSize;
          batchInfo.group = new TangleGroup{0};

          tangle::thread::submitMultipleSync(tests::common::shortTask, offsetValues,
                                             sizeof(values[0]), batchInfo.group, batchSize);
          break;
        }
      case 5:
        tangle::utils::normal << "  " << testIndex \
                              << ": Testing submit multiple, blocked" << std::endl;
        {
          BatchInfo& batchInfo = batchInfoVector.emplace_back();
          batchInfo.waitCount = batchSize;
          batchInfo.group = new TangleGroup{0};

          tangle::thread::blockThreads();
          tangle::thread::submitMultiple(tests::common::shortTask, offsetValues, sizeof(values[0]),
                                         batchInfo.group, batchSize, nullptr);
          tangle::thread::unblockThreads();
          break;
        }
      case 6:
        tangle::utils::normal << "  " << testIndex \
                              << ": Testing chained jobs" << std::endl;
        {
          BatchInfo& batchInfo = batchInfoVector.emplace_back();
          batchInfo.waitCount = batchSize;
          batchInfo.group = new TangleGroup{0};

          tests::common::ChainData* const chainData = new tests::common::ChainData{
            .totalSubmitted = 1, .targetSubmitted = batchSize, .work = tests::common::chainTask,
            .values = offsetValues, .syncPtr = batchInfo.group};
          chainDataVector.push_back(chainData);
          tangle::thread::submitWork(tests::common::chainTask, chainData, batchInfo.group);
          break;
        }
      default:
        std::unreachable();
        break;
      }
    }
    tests::common::finishSubmitTimer(timers);

    //Wait for each batch to finish
    for (const BatchInfo& batchInfo : batchInfoVector) {
      tangle::thread::waitGroupComplete(batchInfo.group, batchInfo.waitCount);
      delete batchInfo.group;
    }

    //Clean up chain data
    for (tests::common::ChainData* const& chainData : chainDataVector) {
      delete chainData;
    }

    tangle::thread::finishWork();
    tests::common::finishExecutionTimers(timers);
    tests::common::printTimers(timers);
    const bool passed = tests::common::verifyWork(totalJobCount, values);

    tests::common::destroyValues(values);
    tests::common::destroyThreadPool();
    tests::common::destroyTimers(timers);
    return passed;
  }
}

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
                                   &group, jobCount, nullptr);
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

namespace tests {
  bool testCreateBlockBlockUnblockUnblockSubmitDestroy(unsigned int jobCount) {
    if (!tests::common::createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);

    tangle::thread::blockThreads();
    tangle::thread::blockThreads();
    tangle::thread::unblockThreads();
    tangle::thread::unblockThreads();

    tests::common::submitShortJobs(jobCount, values);
    tests::common::destroyThreadPool();
    const bool passed = tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    return passed;
  }

  bool testCreateBlockBlockUnblockSubmitDestroy(unsigned int jobCount) {
    if (!tests::common::createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);

    tangle::thread::blockThreads();
    tangle::thread::blockThreads();
    tangle::thread::unblockThreads();

    tests::common::submitShortJobs(jobCount, values);
    tests::common::destroyThreadPool();
    const bool passed = tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    return passed;
  }

  bool testCreateBlockBlockSubmitUnblockDestroy(unsigned int jobCount) {
    if (!tests::common::createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);

    tangle::thread::blockThreads();
    tangle::thread::blockThreads();

    tests::common::submitShortJobs(jobCount, values);
    tangle::thread::unblockThreads();

    tests::common::destroyThreadPool();
    const bool passed = tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    return passed;
  }

  bool testCreateBlockUnblockUnblockSubmitDestroy(unsigned int jobCount) {
    if (!tests::common::createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);

    tangle::thread::blockThreads();
    tangle::thread::unblockThreads();
    tangle::thread::unblockThreads();

    tests::common::submitShortJobs(jobCount, values);
    tests::common::destroyThreadPool();
    const bool passed = tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    return passed;
  }

  bool testCreateUnblockSubmitDestroy(unsigned int jobCount) {
    if (!tests::common::createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = tests::common::createValues(jobCount);

    tangle::thread::unblockThreads();

    tests::common::submitShortJobs(jobCount, values);
    tests::common::destroyThreadPool();
    const bool passed = tests::common::verifyWork(jobCount, values);

    tests::common::destroyValues(values);
    return passed;
  }
}
