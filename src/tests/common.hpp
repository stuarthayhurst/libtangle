#ifndef COMMON
#define COMMON

#include <atomic>
#include <sstream>

#include <tangle/tangle.hpp>

#include "utils/utils.hpp"

namespace tests {
  namespace common {
    //NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables, cppcoreguidelines-interfaces-global-init)
    extern std::stringstream outputCapture;
    //NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables, cppcoreguidelines-interfaces-global-init)
  }

  namespace common {
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

    struct LoggingData {
      unsigned int outputCount;
      std::atomic<unsigned int> index;
      unsigned int* values;
    };

    struct PingPongData {
      TangleGroup* syncPtr;
      void* otherPool;
      PingPongData* otherData;

      std::atomic<unsigned int> totalSubmitted;
      unsigned int targetSubmitted;
      unsigned int* values;
    };

    void shortTask(void* userPtr);
    void resubmitTask(void* userPtr);
    void chainTask(void* userPtr);

    void loggingTask(void* userPtr);

    void blockingTask(void* userPtr);

    void pingPongTask(void* userPtr);
  }

  namespace common {
    bool createThreadPool(unsigned int threadCount);
    void* createThreadPoolInstance(unsigned int threadCount);
    void destroyThreadPool();
    void destroyThreadPool(void* threadPool);

    tests::utils::Timer* createTimers();
    void destroyTimers(const tests::utils::Timer* timers);
    void resetTimers(tests::utils::Timer* timers);
    void resumeSubmitTimer(tests::utils::Timer* timers);
    void finishSubmitTimer(tests::utils::Timer* timers);
    void finishExecutionTimers(tests::utils::Timer* timers);
    void printTimers(const tests::utils::Timer* timers);

    unsigned int* createValues(unsigned int jobCount);
    void destroyValues(const unsigned int* values);

    void submitShortJobs(unsigned int jobCount, unsigned int* values);
    void submitShortSyncJobs(unsigned int jobCount, unsigned int* values, TangleGroup* group);

    bool verifyWork(unsigned int jobCount, const unsigned int* values);
  }
}

#endif
