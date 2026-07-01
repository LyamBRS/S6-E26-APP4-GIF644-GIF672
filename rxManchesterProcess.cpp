#include "Arduino.h"
#include "rxManchesterProcess.h"
#include "rxPacketsProcess.h"
#include "manchesterInterface.h"

//=========================================================
// Private variables
//=========================================================
static QueueHandle_t bitQueue = nullptr;
static SemaphoreHandle_t readySemaphore = nullptr;

//=========================================================
// Private function
//=========================================================
static void processBits(void)
{
  static unsigned char low = 0;
  static unsigned char high = 0;
  static unsigned char bitCount = 0;

  bool bit;

  if (xQueueReceive(bitQueue, &bit, portMAX_DELAY) == pdPASS)
  {
    if (bitCount < 8)
    {
      low = (unsigned char)((low << 1) | (bit ? 1 : 0));
    }
    else
    {
      high = (unsigned char)((high << 1) | (bit ? 1 : 0));
    }

    bitCount++;

    if (bitCount == 16)
    {
      unsigned char decoded = 0;
      int result = manchesterInterface::toUnsignedChar(low, high, &decoded);
      if (result != 0)
      {
        Serial.printf("ERR: rxManchesterProcess::processBits: manchesterInterface::toUnsignedChar: %i\n", result);
      }
      else
      {
        result = rxPacketsProcess::append(decoded);
        if (result != 0 && result != rxPacketsProcess::ERR_QUEUE_FULL)
        {
          Serial.printf("ERR: rxManchesterProcess::processBits: rxPacketsProcess::append: %i\n", result);
        }
      }

      low = 0;
      high = 0;
      bitCount = 0;
    }
  }
}

//=========================================================
// Public definitions
//=========================================================

namespace rxManchesterProcess
{
  int initialize(unsigned char bufferSizeBits)
  {
    bitQueue = xQueueCreate(bufferSizeBits, sizeof(bool));
    if (bitQueue == nullptr)
    {
      return ERR_NULL_QUEUE;
    }

    readySemaphore = xSemaphoreCreateBinary();
    if (readySemaphore == nullptr)
    {
      return ERR_NULL_SEMAPHORE;
    }

    xSemaphoreGive(readySemaphore);

    return 0;
  }

  int append(bool bit)
  {
    if (bitQueue == nullptr)
    {
      return ERR_NULL_QUEUE;
    }

    if (xQueueSend(bitQueue, &bit, 0) != pdPASS)
    {
      return ERR_QUEUE_FULL;
    }

    return 0;
  }

  int getRemainingSpace()
  {
    if (bitQueue == nullptr)
    {
      return ERR_NULL_QUEUE;
    }

    return uxQueueSpacesAvailable(bitQueue);
  }

  int clearBuffer()
  {
    if (bitQueue == nullptr)
    {
      return ERR_NULL_QUEUE;
    }

    xQueueReset(bitQueue);

    return 0;
  }

  void handle(void *pv)
  {
    xSemaphoreTake(readySemaphore, portMAX_DELAY);

    while (true)
    {
      processBits();
    }
  }
}