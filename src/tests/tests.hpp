#ifndef TESTS
#define TESTS

namespace tests {
  //Normal tests
  bool testCreateSubmitWaitDestroy(unsigned int jobCount);
  bool testCreateSubmitBlockUnblockDestroy(unsigned int jobCount);
  bool testCreateSubmitDestroy(unsigned int jobCount);
  bool testCreateBlockSubmitUnblockWaitDestroy(unsigned int jobCount);
  bool testQueueLimits(unsigned int jobCount);
  bool testNestedJobs(unsigned int fullJobCount);
  bool testChainJobs(unsigned int jobCount);
  bool testSingleSyncHelper(unsigned int jobCount);
  bool testMultipleSyncHelper(unsigned int jobCount);

  //Submit multiple tests
  bool testSubmitMultiple(unsigned int jobCount);
  bool testSubmitMultipleMultiple(unsigned int jobCount);
  bool testSubmitMultipleSyncSubmit(unsigned int jobCount);
  bool testSubmitMultipleNoSync(unsigned int jobCount);

  //Varied tests
  bool testRandomWorkloads(unsigned int batchSize);

  //Utility tests
  bool testOutputHelpers(unsigned int jobCount);

  //Blocking tests
  bool testCreateBlockBlockUnblockUnblockSubmitDestroy(unsigned int jobCount);
  bool testCreateBlockBlockUnblockSubmitDestroy(unsigned int jobCount);
  bool testCreateBlockBlockSubmitUnblockDestroy(unsigned int jobCount);
  bool testCreateBlockUnblockUnblockSubmitDestroy(unsigned int jobCount);

  //Confirm functionality
  bool testCreateUnblockSubmitDestroy(unsigned int jobCount);
}

#endif
