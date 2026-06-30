#include "main.h"
#include "schedulerService.h"

namespace SchedulerService
{

  int (*execute[SCHEDULERSERVICE_PHASE_COUNT])(void);

  void initialize(void) {
    for (uint8_t i = 0; i < SCHEDULERSERVICE_PHASE_COUNT; i++)
      execute[i] = neFaitRien;
  }

  int handle(void) {
    for (uint8_t i = 0; i < SCHEDULERSERVICE_PHASE_COUNT; i++) {
      if (execute[i]() < 0) {
            printf("ERROR: SchedulerService - handle - Index %i\n", i);
            return -1;
          }
      }
      return 0;
  }
}