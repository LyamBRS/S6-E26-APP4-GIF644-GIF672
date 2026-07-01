#ifndef TX_SERVICE_H
#define TX_SERVICE_H
///////////////////////////////////////////////////////////
// PURPOSE:
// - Managing the entire transmission of sensors to devices
// - Managing retry functionalities
///////////////////////////////////////////////////////////

namespace txService {

  //=======================================================
  // Public definitions
  //=======================================================
  constexpr int PROCESS_TICK = 10;

  //=======================================================
  // Public functions
  //=======================================================

  int initialize();

  // States
  int state_awaitSensors(void);
  int state_queueTransmission(void);
  int state_awaitTransmissionEnd(void);
  int state_awaitTransmissionACK(void);
  int state_retrying(void);
  
  // RTOS task loop
  void handle(void *pv);
}

#endif