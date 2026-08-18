#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

#include <tangle/tangle.hpp>

#include "tests.hpp"

#include "common.hpp"
#include "utils/utils.hpp"

namespace tests {
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
        tests::common::submitShortSyncJobs(batchSize, offsetValues, NO_GROUP);
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
                                         batchInfo.group, batchSize, NO_GROUP);
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
                                         NO_GROUP, batchSize, batchInfo.group);
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
                                         batchInfo.group, batchSize, NO_GROUP);
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
