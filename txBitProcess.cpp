#include "txBitProcess.h"
#include "manchesterInterface.h"
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
static int count = 0;
static bool arr[16] = {0};

static void processBits(void)
{
  bool bit;

  // Awaits the next bit (blocks only this task)
  if (xQueueReceive(bitQueue, &bit, portMAX_DELAY) == pdPASS)
  {
    digitalWrite(txPin, bit);
    //Serial.print(bit);
    //arr[count] = bit;
    //count++;
//
    //if (count >= 16) {
    //  count = 0;
//
    //  unsigned char low = 0;
    //  unsigned char high = 0;
//
    //  for (int i = 0; i < 8; i++) {
    //    if (arr[i]) {
    //      low |= (1 << i);
    //    }
    //  }
//
    //  for (int i = 0; i < 8; i++) {
    //    if (arr[i + 8]) {
    //      high |= (1 << i);
    //    }
    //  }
//
    //  unsigned char decoded = 0;
    //  int result = manchesterInterface::toUnsignedChar(low, high, &decoded);
    //  if (result != 0)
    //  {
    //    Serial.printf("ERR: Manchester: %i\n", result);
    //    return;
    //  }
    //  Serial.printf(" -> 0x%02X\n", decoded);
    //}
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