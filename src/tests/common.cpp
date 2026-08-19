#include <atomic>
#include <iostream>
#include <sstream>

#include <tangle/tangle.hpp>

#include "common.hpp"

#include "utils/utils.hpp"

namespace tests {
  //NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables, cppcoreguidelines-interfaces-global-init)
  namespace common {
    std::stringstream outputCapture("");

    namespace {
      tangle::utils::OutputHelper outputTester(common::outputCapture, "PREFIX: ");
    }
  }
//NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables, cppcoreguidelines-interfaces-global-init)

  namespace common {
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

    void loggingTask(void* userPtr) {
      LoggingData* const dataPtr = (LoggingData*)userPtr;

      const unsigned int index = dataPtr->index++;
      unsigned int* const valuePtr = &(dataPtr->values[index]);

      outputTester << *valuePtr << " ";
      for (unsigned int i = 0; i < dataPtr->outputCount; i++) {
        outputTester << *valuePtr;
       }
      outputTester << std::endl;

      *valuePtr = 1;
    }

    void blockingTask(void* userPtr) {
      const std::atomic_flag* const flagPtr = (std::atomic_flag*)userPtr;
      flagPtr->wait(false);
    }

    void pingPongTask(void* userPtr) {
      PingPongData* const pingPongData = (PingPongData*)userPtr;

      *(pingPongData->values++) = 1;
      if (pingPongData->totalSubmitted != pingPongData->targetSubmitted) {
        pingPongData->totalSubmitted++;
        tangle::thread::submitWork(pingPongTask, pingPongData->otherData,
                                   pingPongData->syncPtr, pingPongData->otherPool);
      }
    }
  }

  namespace common {
    bool createThreadPool(unsigned int threadCount) {
      if (!tangle::thread::createThreadPool(threadCount)) {
        tangle::utils::error << "Failed to create thread pool, exiting" << std::endl;
        return false;
      }

      return true;
    }

    void* createThreadPoolInstance(unsigned int threadCount) {
      void* const threadPool = tangle::thread::createThreadPoolInstance(threadCount);
      if (threadPool == nullptr) {
        tangle::utils::error << "Failed to create thread pool, exiting" << std::endl;
        return nullptr;
      }

      return threadPool;
    }

    void destroyThreadPool() {
      tangle::thread::destroyThreadPool();
    }

    void destroyThreadPool(void* threadPool) {
      tangle::thread::destroyThreadPool(threadPool);
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
        tangle::thread::submitWork(shortTask, &values[i], NO_GROUP);
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
}
