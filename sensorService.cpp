#include "sensorService.h"
#include "Arduino.h"

//=========================================================
// Private variables
//=========================================================
static SemaphoreHandle_t readySemaphore = nullptr;

//=========================================================
// Private function
//=========================================================

//=========================================================
// Public definitions
//=========================================================
namespace sensorService
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

  String getAll()
  {
    return "Ligne 1 - 2026-06-17 10:00:01 | Temp: 22.4 C | Humidite: 45.2 % | Node: 01\n Ligne 2 - 2026-06-17 10:05:01 | Temp: 22.5 C | Humidite: 45.1 % | Node: 01\nLigne 3 - 2026-06-17 10:10:01 | Temp: 22.8 C | Humidite: 44.9 % | Node: 01\nLigne 4 - 2026-06-17 10:15:01 | Temp: 23.1 C | Humidite: 44.8 % | Node: 01\nLigne 5 - 2026-06-17 10:20:01 | Temp: 23.0 C | Humidite: 45.0 % | Node: 01\n";
  }
  
  void handle(void *pv)
  {
    // wait until system is initialized (optional sync)
    xSemaphoreTake(readySemaphore, portMAX_DELAY);

    while (true)
    {
      vTaskDelay(pdMS_TO_TICKS(PROCESS_TICK));
    }
  }
}