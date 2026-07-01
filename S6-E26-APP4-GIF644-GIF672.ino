#include "txBitProcess.h"
#include "txManchesterProcess.h"
#include "manchesterInterface.h"
//=========================================================
// Program definitions
//=========================================================
#define TX_BIT_BUFFER_SIZE 16
#define PAQUET_MAX_SIZE 80

//=========================================================
// Processes definitions
//=========================================================
TaskHandle_t txBitProcess_TaskHandle = nullptr;
TaskHandle_t txManchesterProcess_TaskHandle = nullptr;

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
  int result = txBitProcess::initialize(TX_BIT_BUFFER_SIZE, 18);
  if (result != 0) {
    Serial.printf("ERR: setup(): txBitProcess::initialize: %i", result);
    return;
  }

  result = txManchesterProcess::initialize(PAQUET_MAX_SIZE);
  if (result != 0) {
    Serial.printf("ERR: setup(): txManchesterProcess::initialize: %i", result);
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

  xTaskCreate(
    txManchesterProcess::handle,
    "txManchesterProcess",
    4096,
    nullptr,
    2,
    &txManchesterProcess_TaskHandle
  );

  //-------------------------------------------------------
  // Pre loop setup
  //-------------------------------------------------------
  Serial.println("Initialization finished");

  delay(2000);
  unsigned char buffer[10] = {0xAA, 0x00, 0xFF, 0xAA, 1, 2, 3, 4, 5, 6};
  
  result = txManchesterProcess::set(buffer, 10);
  if (result != 0) {
    Serial.printf("ERR: setup(): txManchesterProcess::set: %i", result);
  }
}

void loop() {

}
