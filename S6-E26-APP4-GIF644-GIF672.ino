#include "txBitProcess.h"
#include "txManchesterProcess.h"
#include "manchesterInterface.h"
#include "transmissionProcess.h"
//=========================================================
// Program definitions
//=========================================================
#define TX_BIT_BUFFER_SIZE 16
#define PACKET_MAX_SIZE 89

//=========================================================
// Processes definitions
//=========================================================
TaskHandle_t txBitProcess_TaskHandle = nullptr;
TaskHandle_t txManchesterProcess_TaskHandle = nullptr;
TaskHandle_t transmissionProcess_TaskHandle = nullptr;

//=========================================================
// Private variables
//=========================================================

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

  result = txManchesterProcess::initialize(PACKET_MAX_SIZE);
  if (result != 0) {
    Serial.printf("ERR: setup(): txManchesterProcess::initialize: %i", result);
    return;
  }

  result = transmissionProcess::initialize(10);
  if (result != 0) {
    Serial.printf("ERR: setup(): transmissionProcess::initialize: %i", result);
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
    1,
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

  xTaskCreate(
    transmissionProcess::handle,
    "transmissionProcess",
    4096,
    nullptr,
    3,
    &transmissionProcess_TaskHandle
  );

  //-------------------------------------------------------
  // Pre loop setup
  //-------------------------------------------------------
  Serial.println("Initialization finished");

  delay(2000);
  //String test = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ aaaaaaaaaaaaaaaaaaaaaaaa";
  String test = "among";
  
  result = transmissionProcess::send(test);
  if (result != 0) {
    Serial.printf("ERR: setup(): transmissionProcess::set: %i", result);
  }
}

void loop() {

}
