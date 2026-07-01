#include "rxService.h"
#include "rxPacketsProcess.h"
#include "txBitProcess.h"
#include "txManchesterProcess.h"
#include "manchesterInterface.h"
#include "txPacketsProcess.h"
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
TaskHandle_t txPacketsProcess_TaskHandle = nullptr;
TaskHandle_t rxService_TaskHandle = nullptr;

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

  result = txPacketsProcess::initialize(10);
  if (result != 0) {
    Serial.printf("ERR: setup(): txPacketsProcess::initialize: %i", result);
    return;
  }

  result = rxService::initialize(4);
  if (result != 0) {
    Serial.printf("ERR: setup(): rxService::initialize: %i", result);
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
    txPacketsProcess::handle,
    "txPacketsProcess",
    4096,
    nullptr,
    3,
    &txPacketsProcess_TaskHandle
  );

  xTaskCreate(
    rxService::handle,
    "rxService",
    4096,
    nullptr,
    2,
    &rxService_TaskHandle
  );

  //-------------------------------------------------------
  // Pre loop setup
  //-------------------------------------------------------
  Serial.println("Initialization finished");

  delay(2000);
  //String test = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ aaaaaaaaaaaaaaaaaaaaaaaa";
  String test = "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
  
  result = txPacketsProcess::send(test);
  if (result != 0) {
    Serial.printf("ERR: setup(): txPacketsProcess::set: %i", result);
  }
}

void loop() {

  if (rxService::available()) {
    unsigned char message[rxService::MAX_MESSAGE_SIZE] = {0};
    unsigned short messageSize = 0;

    int result = rxService::read(message, &messageSize);
    if (result == 0) {
      Serial.printf("RX message (%u bytes): ", messageSize);
      for (unsigned short i = 0; i < messageSize; i++) {
        Serial.write(message[i]);
      }
      Serial.println();
    }
  }

  if (rxService::retryRequested()) {
    unsigned char retryIndex = 0;
    if (rxService::readRetryRequest(&retryIndex) == 0) {
      Serial.printf("RX retry requested from packet index: %u\n", retryIndex);
    }
  }

}
