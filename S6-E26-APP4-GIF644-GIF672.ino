#include "txBitProcess.h"

//=========================================================
// Processes definitions
//=========================================================
TaskHandle_t txBitProcess_TaskHandle = nullptr;

//=========================================================
// Private variables
//=========================================================
unsigned char count = 0;

//=========================================================
// ARDUINO
//=========================================================
void setup() {
  //-------------------------------------------------------
  // Arduino
  //-------------------------------------------------------
  Serial.begin(115200);

  //-------------------------------------------------------
  // Initializations
  //-------------------------------------------------------
  int result = txBitProcess::initialize(16, 18);
  if (result != 0) {
    Serial.printf("ERR: setup(): txBitProcess::initialize: %i", result);
    return;
  }

  //-------------------------------------------------------
  // Tasks start
  //-------------------------------------------------------
  xTaskCreate(
    txBitProcess::handle,
    "txBitProcess",
    4096,
    nullptr,
    2,
    &txBitProcess_TaskHandle
  );
}

void loop() {

}
