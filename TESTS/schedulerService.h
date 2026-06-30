#ifndef SCHEDULER_SERVICE_H
#define SCHEDULER_SERVICE_H


// --- Software dependencies
//     Copy and paste these in your main, adapting them to your need.
// #define SCHEDULERSERVICE_PHASE_COUNT  0

namespace SchedulerService
{
  // --- Function definitions
  int initialize(void);
  int handle(void);
  extern int (*execute[SCHEDULERSERVICE_PHASE_COUNT])(void);
}
#endif