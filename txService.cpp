#include "txService.h"
#include "Arduino.h"
#include "txPacketsProcess.h"
#include "sensorService.h"

//=========================================================
// Private variables
//=========================================================
static SemaphoreHandle_t readySemaphore = nullptr;
static int (*currentState)(void) = txService::state_awaitSensors;
static String sensorData = "";

//=========================================================
// Private function
//=========================================================

//=========================================================
// Public definitions
//=========================================================
namespace txService
{
  int initialize()
  {
    readySemaphore = xSemaphoreCreateBinary();
    if (readySemaphore == nullptr)
    {
      return ERR_NULL_SEMAPHORE;
    }

    // signal ready once
    xSemaphoreGive(readySemaphore);
    return 0;
  }

  int state_awaitSensors(void)
  {
    // TODO: READY LOGIC
    sensorData = sensorService::getAll();
    currentState = state_queueTransmission;
    return 0;
  }

  int state_queueTransmission(void)
  {
    int result = txPacketsProcess::send(sensorData);
    if (result != 0)
    {
      Serial.printf("ERR: txService: state_queueTransmission: txPacketsProcess::send: %i\n", result);
      return result;
    }
    currentState = state_awaitTransmissionEnd;
    return 0;
  }

  int state_awaitTransmissionEnd(void)
  {
    if (txPacketsProcess::ready())
    {
      currentState = state_awaitTransmissionACK;
    }

    return 0;
  }

  int state_awaitTransmissionACK(void)
  {
    // TODO: Awaited max time? Fuck you.
    // TODO: RX detection -> Ack? yes? go to sensor
    // TODO: RX detection -> no ack? -> retry
    currentState = state_awaitSensors;
    return 0;
  }

  int state_retrying(void)
  {
    int result = txPacketsProcess::retry(0);
    if (result != 0)
    {
      Serial.printf("ERR: txService: state_queueTransmission: txPacketsProcess::send: %i\n", result);
      return result;
    }

    return 0;
  }

  void handle(void *pv)
  {
    // wait until system is initialized (optional sync)
    xSemaphoreTake(readySemaphore, portMAX_DELAY);

    while (true)
    {
      int result = currentState();
      if (result != 0) {
        Serial.printf("ERR: txService: handle: currentState: %i\n", result);
      }
      vTaskDelay(pdMS_TO_TICKS(PROCESS_TICK));
    }
  }
}