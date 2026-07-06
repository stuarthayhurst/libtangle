#include <tangle/tangle.hpp>

#include "tests.hpp"

#include "common.hpp"

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
