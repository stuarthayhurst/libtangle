#include <cstdlib>
#include <iostream>
#include <string>

#include <tangle/tangle.hpp>

#include "tests/tests.hpp"
#include "tests/utils/utils.hpp"

namespace {
  //Test type signature
  using TestPointer = bool (*)(unsigned int jobCount);

  //Enums to determine how the job count is handled
  enum JobCountMode : unsigned char {
    DEFAULT,
    SINGLE,
    POOL_SIZE,
    HARDWARE_SIZE_4
  };

  //NOLINTBEGIN(modernize-use-designated-initializers)
  struct TestInfo {
    std::string title;
    TestPointer testPointer;
    JobCountMode jobCountMode;
  } testInfoArray[] = {
    //Normal tests
    {"Testing standard submit, wait, destroy", tests::testCreateSubmitWaitDestroy, DEFAULT},
    {"Testing alternative sync", tests::testCreateSubmitBlockUnblockDestroy, DEFAULT},
    {"Testing no sync", tests::testCreateSubmitDestroy, DEFAULT},
    {"Testing blocked queue", tests::testCreateBlockSubmitUnblockWaitDestroy, DEFAULT},
    {"Testing queue limits (8x regular over 2 batches)", tests::testQueueLimits, DEFAULT},
    {"Testing nested jobs", tests::testNestedJobs, DEFAULT},
    {"Testing chained jobs", tests::testChainJobs, DEFAULT},
    {"Testing single synchronisation helper", tests::testSingleSyncHelper, DEFAULT},
    {"Testing multiple synchronisation helpers", tests::testMultipleSyncHelper, DEFAULT},

    //Submit multiple tests
    {"Testing submit multiple", tests::testSubmitMultiple, DEFAULT},
    {"Testing submit multiple, minimal", tests::testSubmitMultiple, SINGLE},
    {"Testing submit multiple, pool size", tests::testSubmitMultiple, POOL_SIZE},
    {"Testing submit multiple (4x regular over 4 batches)", tests::testSubmitMultipleMultiple,
     DEFAULT},
    {"Testing submit multiple, synchronous submit", tests::testSubmitMultipleSyncSubmit, DEFAULT},
    {"Testing submit multiple, no job sync", tests::testSubmitMultipleNoSync, DEFAULT},

    //Varied tests
    {"Testing random workloads", tests::testRandomWorkloads, DEFAULT},

    //Utility tests
    {"Testing synchronised output helpers", tests::testOutputHelpers, HARDWARE_SIZE_4},

    //Blocking tests
    {"Testing double block, double unblock", tests::testCreateBlockBlockUnblockUnblockSubmitDestroy,
     DEFAULT},
    {"Testing double block, single unblock", tests::testCreateBlockBlockUnblockSubmitDestroy,
     DEFAULT},
    {"Testing double block, submit jobs, single unblock",
     tests::testCreateBlockBlockSubmitUnblockDestroy, DEFAULT},
    {"Testing single block, double unblock", tests::testCreateBlockUnblockUnblockSubmitDestroy,
     DEFAULT},
    {"Testing unblock without block", tests::testCreateUnblockSubmitDestroy, DEFAULT},

    //Multi-pool tests
    {"Testing multi-pool ping pong", tests::testPingPongPools, DEFAULT},
    {"Testing suspended jobs", tests::testSuspendedJob, DEFAULT},

    //Confirm functionality
    {"Double-checking standard submit, wait, destroy", tests::testCreateSubmitWaitDestroy, DEFAULT}
  };
  //NOLINTEND(modernize-use-designated-initializers)
}

namespace {
  struct TestStats {
    unsigned int passed = 0;
    unsigned int total = 0;
  };

  void runTest(const TestInfo& testInfo, unsigned int jobCount, TestStats* testStatsPtr) {
    //Print the test title
    tangle::utils::normal << testInfo.title << std::endl;

    //Handle the job count mode
    switch (testInfo.jobCountMode) {
    case DEFAULT:
      break;
    case SINGLE:
      jobCount = 1;
      break;
    case POOL_SIZE:
      jobCount = tangle::thread::getThreadPoolSize();
      break;
    case HARDWARE_SIZE_4:
      jobCount = tangle::thread::getHardwareThreadCount() * 4;
      break;
    }

    //Run the test
    const bool passed = testInfo.testPointer(jobCount);

    //Update the test stats
    testStatsPtr->total++;
    if (passed) {
      testStatsPtr->passed++;
    }
  }

  void printSummary(const TestStats& testStats, const tests::utils::Timer& totalTimer) {
    tangle::utils::normal.printEmptyLine();
    tangle::utils::normal << "Tests passed: " << testStats.passed << " / " \
                          << testStats.total << std::endl;
    tangle::utils::normal << "Tests failed: " << testStats.total - testStats.passed \
                          << " / " << testStats.total << std::endl;
    tangle::utils::normal << "Total time: " << totalTimer.getTime() << "s" << std::endl;
  }
}

int main() noexcept(false) {
  tangle::utils::status << tangle::thread::getHardwareThreadCount() \
                        << " hardware threads detected" << std::endl;

  //Pick jobs per test
  const unsigned int jobCount = (2 << 16);

  //Run the tests
  const tests::utils::Timer totalTimer;
  TestStats testStats;
  for (const TestInfo& testInfo : testInfoArray) {
    runTest(testInfo, jobCount, &testStats);
  }

  //Print test summary
  printSummary(testStats, totalTimer);

  const bool passed = (testStats.passed == testStats.total);
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
