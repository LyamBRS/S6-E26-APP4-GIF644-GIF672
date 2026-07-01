#include "txBitProcess.h"
#include "Arduino.h"

//=========================================================
// Private variables
//=========================================================
static QueueHandle_t bitQueue = nullptr;
static SemaphoreHandle_t readySemaphore = nullptr;

static int txPin = -1;

//=========================================================
// Private function
//=========================================================

static void processBits(void)
{
  bool bit;

  // Awaits the next bit (blocks only this task)
  if (xQueueReceive(bitQueue, &bit, portMAX_DELAY) == pdPASS)
  {
    digitalWrite(txPin, bit);
  }
}

//=========================================================
// Public definitions
//=========================================================

namespace txBitProcess
{
  int initialize(unsigned char buffer_size, int tx_pin)
  {
    // --- Hardware management
    txPin = tx_pin;
    pinMode(txPin, OUTPUT);

    // --- RTOS management
    bitQueue = xQueueCreate(buffer_size, sizeof(bool));
    if (bitQueue == nullptr)
    {
      return ERR_NULL_QUEUE;
    }

    readySemaphore = xSemaphoreCreateBinary();
    if (readySemaphore == nullptr)
    {
      return ERR_NULL_SEMAPHORE;
    }

    // signal ready once
    xSemaphoreGive(readySemaphore);

    return 0;
  }

  int getRemainingSpace()
  {
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

  int append(unsigned char bits, unsigned char amount)
  {
    if (amount > 8)
    {
      return ERR_AMOUNT_TOO_HIGH;
    }

    for (int i = amount-1; i >= 0; --i)
    {
      bool bit = (bits >> i) & 1;

      if (xQueueSend(bitQueue, &bit, 0) != pdPASS)
      {
        return ERR_QUEUE_FULL;
      }
    }

    return 0;
  }

  void handle(void *pv)
  {
    // wait until system is initialized (optional sync)
    xSemaphoreTake(readySemaphore, portMAX_DELAY);

    while (true)
    {
      processBits();
      vTaskDelay(pdMS_TO_TICKS(PROCESS_TICK));
    }
  }
}