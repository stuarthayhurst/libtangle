#include <iostream>

#include "group.hpp"

#include "debug.hpp"

namespace tangle {
  namespace thread {
    namespace internal {
      //Wait for jobCount jobs in group to finish
      void waitGroupComplete(TangleGroup* group, unsigned int jobCount) {
        for (unsigned int i = 0; i < jobCount; i++) {
          group->acquire();
        }
      }

      /*
       - Return true if at least one item of a group has finished
         - May spuriously fail, returning false when work had finished
       - Acts like synchronisation if successful, decreasing the group's counter
      */
      bool isSingleWorkComplete(TangleGroup* group) {
        return group->try_acquire();
      }

      /*
       - Return the number of unfinished jobs in a group
         - This is jobCount - the number of successfully acquired jobs
         - Remaining work may be overestimated, but never underestimated
       - Acts like synchronisation if successful, decreasing the group's counter
         - This means the return value can't be ignored if the group will be used for
           synchronisation later on
      */
      unsigned int getRemainingWork(TangleGroup* group, unsigned int jobCount) {
        unsigned int finishedJobs = 0;
        while (group->try_acquire()) {
          finishedJobs++;
        };

        return jobCount - finishedJobs;
      }
    }
  }
}

namespace tangle {
  namespace thread {
    /*
     - Wait for a group to be finished
     - jobCount determines how many jobs to wait for
       - If less than jobCount jobs have been given the group, this will block forever
       - It doesn't matter if the jobs have already finished
    */
    void waitGroupComplete(TangleGroup* group, unsigned int jobCount) {
      if (group != nullptr) {
        internal::waitGroupComplete(group, jobCount);
      } else {
        tangleInternalDebug << "Group is a nullptr, skipping wait" << std::endl;
      }
    }

    /*
     - Check if at least one item of a group has finished
       - May spuriously fail, returning false when work had finished
     - Acts like synchronisation if successful, decreasing the group's counter
       - A second call to a group with 1 complete work item would return false
       - Using waitGroupComplete() at this point would block
     - If unsuccessful, nothing in the group is modified
    */
    bool isSingleWorkComplete(TangleGroup* group) {
      if (group != nullptr) {
        return internal::isSingleWorkComplete(group);
      }

      tangleInternalDebug << "Group is a nullptr, skipping check" << std::endl;
      return false;
    }

    /*
     - Return the number of unfinished jobs in a group
     - Successive calls should use the remaining jobs returned as the job count
       - Subtract any synchronised / successfully queried jobs from this too
     - Remaining work may be overestimated, but never underestimated
     - Acts like synchronisation if successful, decreasing the group's counter
     - If unsuccessful, nothing in the group is modified
    */
    unsigned int getRemainingWork(TangleGroup* group, unsigned int jobCount) {
      if (group != nullptr) {
        return internal::getRemainingWork(group, jobCount);
      }

      tangleInternalDebug << "Group is a nullptr, skipping query" << std::endl;
      return jobCount;
    }
  }
}
