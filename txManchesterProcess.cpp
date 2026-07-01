#include "Arduino.h"
#include "txManchesterProcess.h"
#include "txBitProcess.h"
#include "manchesterInterface.h"

//=========================================================
// Private variables
//=========================================================
static QueueHandle_t txQueue;
static SemaphoreHandle_t readySemaphore = nullptr;
//static SemaphoreHandle_t txLock = nullptr;
static volatile bool aborted = false;
static bool txLock = false;

//=========================================================
// Private function
//=========================================================
void manageQueue()
{
  unsigned char  byte;
  while (txBitProcess::getRemainingSpace() >= 16) {
    if (uxQueueMessagesWaiting(txQueue) == 0)
    {
      txLock = false;
    }

    if (xQueueReceive(txQueue, &byte, portMAX_DELAY) == pdPASS)
    {
      Serial.printf("Queuing -> 0x%02X\n", byte);
      unsigned char high = 0;
      unsigned char low = 0;
      int result = manchesterInterface::fromUnsignedChar(byte, &low, &high);
      if (result != 0) {
        Serial.printf("ERR: txManchesterProcess.cpp: manchesterInterface::fromUnsignedChar: %i", result);
      }
      
      result = txBitProcess::append(low, 8);
      if (result != 0) {
        Serial.printf("ERR: txManchesterProcess.cpp: txBitProcess::append: %i", result);
      }
      result = txBitProcess::append(high, 8);
      if (result != 0) {
        Serial.printf("ERR: txManchesterProcess.cpp: txBitProcess::append: %i", result);
      }

      vTaskDelay(pdMS_TO_TICKS(10));
    }
    else
    {
      // queue empty → finished
      Serial.println("queue finished");
      //xSemaphoreGive(txLock);
      txLock = false;
      return;
    }
  }
}

//=========================================================
// Public definitions
//=========================================================

namespace txManchesterProcess
{
  int initialize(unsigned char bufferSizeBytes)
  {
    // --- RTOS management
    txQueue = xQueueCreate(bufferSizeBytes, sizeof(unsigned char));
    if (txQueue == nullptr)
    {
      return ERR_NULL_QUEUE;
    }

    readySemaphore = xSemaphoreCreateBinary();
    if (readySemaphore == nullptr)
    {
      return ERR_NULL_SEMAPHORE;
    }

    //txLock = xSemaphoreCreateMutex();
    //if (txLock == nullptr)
    //{
    //  return ERR_NULL_SEMAPHORE;
    //}
    txLock = false;

    // signal ready once
    xSemaphoreGive(readySemaphore);
    //xSemaphoreGive(txLock);

    return 0;
  }

  int set(const unsigned char* packet, unsigned char byteCount)
  {
    // - Data can't be appended until whole packet is sent.
    //if (xSemaphoreTake(txLock, 0) != pdTRUE)
    //{
    //  return ERR_BUSY;
    //}
    if (txLock) {
      return ERR_BUSY;
    }
    txLock = true;

    // - Previous queue clearing
    xQueueReset(txQueue);

    aborted = false;

    for (size_t i = 0; i < byteCount; i++)
    {
      if (xQueueSend(txQueue, &packet[i], 0) != pdPASS)
      {
        txLock = false;
        return ERR_BUFFER_OVERFLOW;
      }
    }

    // - Process can now run
    //xSemaphoreGive(txLock);

    return 0;
  }

  void handle(void *pv)
  {
    while(true)
    {
      if (aborted)
      {
        xQueueReset(txQueue);
        xSemaphoreGive(txLock);
        continue;
      }
      manageQueue();
      vTaskDelay(pdMS_TO_TICKS(PROCESS_TICK));
    }

    Serial.println("FATAL: txManchgesterProcess: while exit.");
  }
}