#ifndef SENSOR_SERVICE_H
#define SENSOR_SERVICE_H
///////////////////////////////////////////////////////////
// PURPOSE:
// - Gather data from all the sensors
// - Create ready made sensor messages
///////////////////////////////////////////////////////////

#include "Arduino.h"

namespace sensorService {

  //=======================================================
  // Public definitions
  //=======================================================
  constexpr int ERR_NULL_SEMAPHORE = -1;
  constexpr int PROCESS_TICK = 100;

  //=======================================================
  // Public functions
  //=======================================================

  int initialize();

  // States
  String getAll(void);
  
  // RTOS task loop
  void handle(void *pv);
}

#endif