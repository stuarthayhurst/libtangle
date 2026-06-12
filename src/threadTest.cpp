#include <atomic>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <tangle/tangle.hpp>

#include "tests/utils/utils.hpp"

namespace {
  //NOLINTBEGIN(cppcoreguidelines-interfaces-global-init)
  std::stringstream outputCapture("");
  tangle::utils::OutputHelper outputTester(outputCapture, "PREFIX: ");
  //NOLINTEND(cppcoreguidelines-interfaces-global-init)
}

namespace {
  struct ResubmitData {
    unsigned int* writePtr;
    TangleGroup* syncPtr;
  };

  struct ChainData {
    std::atomic<unsigned int> totalSubmitted;
    unsigned int targetSubmitted;
    TangleWork work;
    unsigned int* values;
    TangleGroup* syncPtr;
  };

  void shortTask(void* userPtr) {
    *(unsigned int*)userPtr = 1;
  }

  void resubmitTask(void* userPtr) {
    const ResubmitData* const dataPtr = (ResubmitData*)userPtr;
    tangle::thread::submitWork(shortTask, dataPtr->writePtr, dataPtr->syncPtr);
  }

  void chainTask(void* userPtr) {
    ChainData* const dataPtr = (ChainData*)userPtr;
    *(dataPtr->values++) = 1;
    if (dataPtr->totalSubmitted != dataPtr->targetSubmitted) {
      dataPtr->totalSubmitted++;
      tangle::thread::submitWork(chainTask, dataPtr, dataPtr->syncPtr);
    }
  }

  constexpr unsigned int outputCount = 1000;
  void loggingTask(void* userPtr) {
    const unsigned int value = *((unsigned int*)userPtr);
    outputTester << value << " ";
    for (unsigned int i = 0; i < outputCount; i++) {
      outputTester << value;
    }
    outputTester << std::endl;

    *(unsigned int*)userPtr = 1;
  }

  void blockingTask(void* userPtr) {
    const std::atomic_flag* const flagPtr = (std::atomic_flag*)userPtr;
    flagPtr->wait(false);
  }
}

namespace {
  bool createThreadPool(unsigned int threadCount) {
    if (!tangle::thread::createThreadPool(threadCount)) {
      tangle::utils::error << "Failed to create thread pool, exiting" << std::endl;
      return false;
    }

    return true;
  }

  void destroyThreadPool() {
    tangle::thread::destroyThreadPool();
  }

  tests::utils::Timer* createTimers() {
    return new tests::utils::Timer[3];
  }

  void destroyTimers(tests::utils::Timer* timers) {
    delete [] timers;
  }

  void resetTimers(tests::utils::Timer* timers) {
    for (int i = 0; i < 3; i++) {
      timers[i].reset();
    }
  }

  void resumeSubmitTimer(tests::utils::Timer* timers) {
    timers[0].unpause();
  }

  void finishSubmitTimer(tests::utils::Timer* timers) {
    timers[0].pause();
  }

  void finishExecutionTimers(tests::utils::Timer* timers) {
    timers[1].pause();
    timers[2].pause();
  }

  void printTimers(tests::utils::Timer* timers) {
    tangle::utils::normal << "  Submit done : " << timers[0].getTime() << "s" << std::endl;
    tangle::utils::normal << "  Finish work : " << timers[1].getTime() << "s" << std::endl;
    tangle::utils::normal << "  Total time  : " << timers[2].getTime() << "s" << std::endl;
  }

  unsigned int* createValues(unsigned int jobCount) {
    return new unsigned int[jobCount]{};
  }

  void destroyValues(const unsigned int* values) {
    delete [] values;
  }

  void submitShortJobs(unsigned int jobCount, unsigned int* values) {
    for (unsigned int i = 0; i < jobCount; i++) {
      tangle::thread::submitWork(shortTask, &values[i], nullptr);
    }
  }

  void submitShortSyncJobs(unsigned int jobCount, unsigned int* values, TangleGroup* group) {
    for (unsigned int i = 0; i < jobCount; i++) {
      tangle::thread::submitWork(shortTask, &values[i], group);
    }
  }

  bool verifyWork(unsigned int jobCount, const unsigned int* values) {
    for (unsigned int i = 0; i < jobCount; i++) {
      if (values[i] != 1) {
        tangle::utils::error << "Failed to verify work (index " << i << ")" << std::endl;
        return false;
      }
    }

    return true;
  }
}

namespace {
  bool testCreateSubmitWaitDestroy(unsigned int jobCount) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    TangleGroup group{0};

    //Submit fast 'jobs'
    resetTimers(timers);
    submitShortSyncJobs(jobCount, values, &group);
    finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testCreateSubmitBlockUnblockDestroy(unsigned int jobCount) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    //Submit fast 'jobs'
    resetTimers(timers);
    submitShortJobs(jobCount, values);
    finishSubmitTimer(timers);

    //Finish work
    tangle::thread::finishWork();
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testCreateSubmitDestroy(unsigned int jobCount) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    //Submit fast 'jobs'
    resetTimers(timers);
    submitShortJobs(jobCount, values);
    finishSubmitTimer(timers);

    //Finish work
    destroyThreadPool();
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyTimers(timers);
    return passed;
  }

  bool testCreateBlockSubmitUnblockWaitDestroy(unsigned int jobCount) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    TangleGroup group{0};

    tangle::thread::blockThreads();

    //Submit fast 'jobs'
    resetTimers(timers);
    submitShortSyncJobs(jobCount, values, &group);
    finishSubmitTimer(timers);

    //Finish work
    tangle::thread::unblockThreads();
    tangle::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testQueueLimits(unsigned int jobCount) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    jobCount *= 4;
    unsigned int* values = createValues(jobCount);
    TangleGroup group{0};

    //Submit fast 'jobs'
    resetTimers(timers);
    submitShortSyncJobs(jobCount, values, &group);
    finishSubmitTimer(timers);

    //Clean up after the first batch
    tangle::thread::waitGroupComplete(&group, jobCount);
    bool passed = verifyWork(jobCount, values);
    destroyValues(values);

    //Submit second batch
    values = createValues(jobCount);
    resumeSubmitTimer(timers);
    submitShortSyncJobs(jobCount, values, &group);
    finishSubmitTimer(timers);

    //Clean up after the second batch
    tangle::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    passed &= verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testNestedJobs(int fullJobCount) {
    const unsigned int jobCount = fullJobCount / 2;
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    TangleGroup group{0};

    //Submit nested 'jobs'
    resetTimers(timers);
    ResubmitData* const data = new ResubmitData[jobCount]{};
    for (unsigned int i = 0; i < jobCount; i++) {
      data[i].writePtr = &values[i];
      data[i].syncPtr = &group;
      tangle::thread::submitWork(resubmitTask, &data[i], nullptr);
    }
    finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    delete [] data;
    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testChainJobs(unsigned int jobCount) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    const unsigned int poolSize = tangle::thread::getThreadPoolSize();
    const unsigned int totalJobCount = jobCount * poolSize;
    unsigned int* const values = createValues(totalJobCount);
    TangleGroup sync{0};

    ChainData* const userDataArray = new ChainData[poolSize];
    for (std::size_t i = 0; i < poolSize; i++) {
      userDataArray[i].totalSubmitted = 1;
      userDataArray[i].targetSubmitted = jobCount;
      userDataArray[i].work = chainTask;
      userDataArray[i].values = values + (i * jobCount);
      userDataArray[i].syncPtr = &sync;
    }

    //Submit chain 'jobs'
    resetTimers(timers);
    tangle::thread::submitMultiple(chainTask, userDataArray, sizeof(ChainData),
                                   &sync, poolSize, nullptr);
    finishSubmitTimer(timers);

    tangle::thread::waitGroupComplete(&sync, totalJobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(totalJobCount, values);

    delete [] userDataArray;
    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testSingleSyncHelper(unsigned int jobCount) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
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
    resetTimers(timers);
    for (unsigned int i = 0; i < jobCount; i++) {
      tangle::thread::submitWork(blockingTask, &flags[i], &group);
    }
    finishSubmitTimer(timers);

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

    finishExecutionTimers(timers);
    printTimers(timers);

    delete [] flags;
    destroyThreadPool();
    destroyTimers(timers);
    return true;
  }

  bool testMultipleSyncHelper(unsigned int jobCount) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    TangleGroup group{0};

    //Test multiple work complete check before work is submitted
    if (tangle::thread::getRemainingWork(&group, jobCount) != jobCount) {
      tangle::utils::error << "Work incorrectly reported as complete" << std::endl;
      return false;
    }

    //Submit 'fast' jobs
    resetTimers(timers);
    tangle::thread::submitMultiple(shortTask, &values[0], sizeof(values[0]),
                                   &group, jobCount, nullptr);
    finishSubmitTimer(timers);

    //Wait for the jobs to complete
    unsigned int remainingJobs = jobCount;
    while (remainingJobs != 0) {
      remainingJobs = tangle::thread::getRemainingWork(&group, remainingJobs);
    }

    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testSubmitMultiple(unsigned int jobCount) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    TangleGroup group{0};

    //Submit fast 'jobs'
    resetTimers(timers);
    tangle::thread::submitMultiple(shortTask, &values[0], sizeof(values[0]),
                                   &group, jobCount, nullptr);
    finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testSubmitMultipleMultiple(unsigned int jobCount) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount * 4);
    TangleGroup group{0};

    //Submit fast 'jobs'
    resetTimers(timers);
    for (int i = 0; i < 4; i++) {
      tangle::thread::submitMultiple(shortTask, &values[(std::size_t)jobCount * i],
                                     sizeof(values[0]), &group, jobCount, nullptr);
    }
    finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&group, jobCount * 4);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount * 4, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testSubmitMultipleSyncSubmit(unsigned int jobCount) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    TangleGroup group{0};

    //Submit fast 'jobs'
    resetTimers(timers);
    tangle::thread::submitMultipleSync(shortTask, &values[0], sizeof(values[0]),
                                       &group, jobCount);
    finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }

  bool testSubmitMultipleNoSync(unsigned int jobCount) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    unsigned int* const values = createValues(jobCount);
    TangleGroup submitGroup{0};

    //Submit fast 'jobs'
    resetTimers(timers);
    tangle::thread::submitMultiple(shortTask, &values[0], sizeof(values[0]),
                                   nullptr, jobCount, &submitGroup);
    finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&submitGroup, 1);
    destroyThreadPool();
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    destroyTimers(timers);
    return passed;
  }

  bool testRandomWorkloads(unsigned int batchSize) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    const unsigned int testCount = 15;
    const unsigned int totalJobCount = batchSize * testCount;
    unsigned int* const values = createValues(totalJobCount);

    struct BatchInfo {
      TangleGroup* group;
      unsigned int waitCount = 0;
    };

    resetTimers(timers);
    std::vector<BatchInfo> batchInfoVector;
    std::vector<ChainData*> chainDataVector;
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

          submitShortSyncJobs(batchSize, offsetValues, batchInfo.group);
          break;
        }
      case 1:
        tangle::utils::normal << "  " << testIndex \
                              << ": Testing regular submit, without explicit sync" \
                              << std::endl;
        submitShortSyncJobs(batchSize, offsetValues, nullptr);
        break;
      case 2:
        tangle::utils::normal << "  " << testIndex \
                              << ": Testing submit multiple, sync on jobs" \
                              << std::endl;
        {
          BatchInfo& batchInfo = batchInfoVector.emplace_back();
          batchInfo.waitCount = batchSize;
          batchInfo.group = new TangleGroup{0};

          tangle::thread::submitMultiple(shortTask, offsetValues, sizeof(values[0]),
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

          tangle::thread::submitMultiple(shortTask, offsetValues, sizeof(values[0]),
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

          tangle::thread::submitMultipleSync(shortTask, offsetValues, sizeof(values[0]),
                                             batchInfo.group, batchSize);
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
          tangle::thread::submitMultiple(shortTask, offsetValues, sizeof(values[0]),
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

          ChainData* const chainData = new ChainData{
            .totalSubmitted = 1, .targetSubmitted = batchSize, .work = chainTask,
            .values = offsetValues, .syncPtr = batchInfo.group};
          chainDataVector.push_back(chainData);
          tangle::thread::submitWork(chainTask, chainData, batchInfo.group);
          break;
        }
      default:
        std::unreachable();
        break;
      }
    }
    finishSubmitTimer(timers);

    //Wait for each batch to finish
    for (const BatchInfo& batchInfo : batchInfoVector) {
      tangle::thread::waitGroupComplete(batchInfo.group, batchInfo.waitCount);
      delete batchInfo.group;
    }

    //Clean up chain data
    for (ChainData* const& chainData : chainDataVector) {
      delete chainData;
    }

    tangle::thread::finishWork();
    finishExecutionTimers(timers);
    printTimers(timers);
    const bool passed = verifyWork(totalJobCount, values);

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }
}

namespace {
  bool testOutputHelpers(unsigned int jobCount) {
    tests::utils::Timer* const timers = createTimers();
    if (!createThreadPool(0)) {
      destroyTimers(timers);
      return false;
    }
    TangleGroup group{0};

    //Submit logging jobs
    resetTimers(timers);
    unsigned int* const values = createValues(jobCount);
    for (unsigned int i = 0; i < jobCount; i++) {
      values[i] = i;
      tangle::thread::submitWork(loggingTask, &values[i], &group);
    }
    finishSubmitTimer(timers);

    //Finish work
    tangle::thread::waitGroupComplete(&group, jobCount);
    finishExecutionTimers(timers);
    printTimers(timers);
    bool passed = verifyWork(jobCount, values);

    //Verify output blocks
    std::string threadOutput;
    std::unordered_set<unsigned int> foundValues;
    while (std::getline(outputCapture, threadOutput)) {
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

    destroyValues(values);
    destroyThreadPool();
    destroyTimers(timers);
    return passed;
  }
}

namespace {
  bool testCreateBlockBlockUnblockUnblockSubmitDestroy(unsigned int jobCount) {
    if (!createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    tangle::thread::blockThreads();
    tangle::thread::blockThreads();
    tangle::thread::unblockThreads();
    tangle::thread::unblockThreads();

    submitShortJobs(jobCount, values);
    destroyThreadPool();
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    return passed;
  }

  bool testCreateBlockBlockUnblockSubmitDestroy(unsigned int jobCount) {
    if (!createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    tangle::thread::blockThreads();
    tangle::thread::blockThreads();
    tangle::thread::unblockThreads();

    submitShortJobs(jobCount, values);
    destroyThreadPool();
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    return passed;
  }

  bool testCreateBlockBlockSubmitUnblockDestroy(unsigned int jobCount) {
    if (!createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    tangle::thread::blockThreads();
    tangle::thread::blockThreads();

    submitShortJobs(jobCount, values);
    tangle::thread::unblockThreads();

    destroyThreadPool();
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    return passed;
  }

  bool testCreateBlockUnblockUnblockSubmitDestroy(unsigned int jobCount) {
    if (!createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    tangle::thread::blockThreads();
    tangle::thread::unblockThreads();
    tangle::thread::unblockThreads();

    submitShortJobs(jobCount, values);
    destroyThreadPool();
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    return passed;
  }

  bool testCreateUnblockSubmitDestroy(unsigned int jobCount) {
    if (!createThreadPool(0)) {
      return false;
    }
    unsigned int* const values = createValues(jobCount);

    tangle::thread::unblockThreads();

    submitShortJobs(jobCount, values);
    destroyThreadPool();
    const bool passed = verifyWork(jobCount, values);

    destroyValues(values);
    return passed;
  }
}

int main() noexcept(false) {
  bool failed = false;
  tangle::utils::status << tangle::thread::getHardwareThreadCount() \
                        << " hardware threads detected" << std::endl;

  //Pick jobs per test
  const unsigned int jobCount = (2 << 16);

  //Begin regular tests
  tangle::utils::normal << "Testing standard submit, wait, destroy" << std::endl;
  failed |= !testCreateSubmitWaitDestroy(jobCount);

  tangle::utils::normal << "Testing alternative sync" << std::endl;
  failed |= !testCreateSubmitBlockUnblockDestroy(jobCount);

  tangle::utils::normal << "Testing no sync" << std::endl;
  failed |= !testCreateSubmitDestroy(jobCount);

  tangle::utils::normal << "Testing blocked queue" << std::endl;
  failed |= !testCreateBlockSubmitUnblockWaitDestroy(jobCount);

  tangle::utils::normal << "Testing queue limits (8x regular over 2 batches)" << std::endl;
  failed |= !testQueueLimits(jobCount);

  tangle::utils::normal << "Testing nested jobs" << std::endl;
  failed |= !testNestedJobs(jobCount);

  tangle::utils::normal << "Testing chained jobs" << std::endl;
  failed |= !testChainJobs(jobCount);

  tangle::utils::normal << "Testing single synchronisation helper" << std::endl;
  failed |= !testSingleSyncHelper(jobCount);

  tangle::utils::normal << "Testing multiple synchronisation helpers" << std::endl;
  failed |= !testMultipleSyncHelper(jobCount);

  tangle::utils::normal << "Testing submit multiple" << std::endl;
  failed |= !testSubmitMultiple(jobCount);

  tangle::utils::normal << "Testing submit multiple, minimal" << std::endl;
  failed |= !testSubmitMultiple(1);

  tangle::utils::normal << "Testing submit multiple, thread count" << std::endl;
  failed |= !testSubmitMultiple(tangle::thread::getThreadPoolSize());

  tangle::utils::normal << "Testing submit multiple (4x regular over 4 batches)" << std::endl;
  failed |= !testSubmitMultipleMultiple(jobCount);

  tangle::utils::normal << "Testing submit multiple, synchronous submit" << std::endl;
  failed |= !testSubmitMultipleSyncSubmit(jobCount);

  tangle::utils::normal << "Testing submit multiple, no job sync" << std::endl;
  failed |= !testSubmitMultipleNoSync(jobCount);

  tangle::utils::normal << "Testing random workloads" << std::endl;
  failed |= !testRandomWorkloads(jobCount);

  tangle::utils::normal << "Testing synchronised output helpers" << std::endl;
  const unsigned int threadCount = tangle::thread::getHardwareThreadCount();
  failed |= !testOutputHelpers(threadCount * 4);

  //Begin blocking tests
  tangle::utils::normal << "Testing double block, double unblock" << std::endl;
  failed |= !testCreateBlockBlockUnblockUnblockSubmitDestroy(jobCount);

  tangle::utils::normal << "Testing double block, single unblock" << std::endl;
  failed |= !testCreateBlockBlockUnblockSubmitDestroy(jobCount);

  tangle::utils::normal << "Testing double block, submit jobs, single unblock" << std::endl;
  failed |= !testCreateBlockBlockSubmitUnblockDestroy(jobCount);

  tangle::utils::normal << "Testing single block, double unblock" << std::endl;
  failed |= !testCreateBlockUnblockUnblockSubmitDestroy(jobCount);

  tangle::utils::normal << "Testing unblock without block" << std::endl;
  failed |= !testCreateUnblockSubmitDestroy(jobCount);

  //Check system is still functional
  tangle::utils::normal << "Double-checking standard submit, wait, destroy" << std::endl;
  failed |= !testCreateSubmitWaitDestroy(jobCount);

  return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
