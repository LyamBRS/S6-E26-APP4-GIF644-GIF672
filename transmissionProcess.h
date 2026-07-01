#ifndef TRANSMISSION_PROCESS_H
#define TRANSMISSION_PROCESS_H

#include "Arduino.h"

///////////////////////////////////////////////////////////
// PURPOSE:
// - Managing given data as packets
// - Adding delimiters and stuff
// - Calculating CRC16
// - Queueing up packets to send
///////////////////////////////////////////////////////////

namespace transmissionProcess {

  //=======================================================
  // Public definitions
  //=======================================================
  constexpr int ERR_NULL_QUEUE = -1;
  constexpr int ERR_NULL_SEMAPHORE = -2;
  constexpr int ERR_BUSY = -3;
  constexpr int ERR_BUFFER_OVERFLOW = -4;
  constexpr int ERR_RETRY_PACKET_INDEX = -5;
  constexpr int ERR_OUT_OF_MEMORY = -6;
  constexpr int ERR_ALREADY_INITIALIZED = -7;
  constexpr int ERR_INVALID_INDEX = -8;
  constexpr int ERR_PARAMETERS_NULL = -9;
  constexpr int ERR_NOT_TRANSMITTING = -10;

  constexpr int PROCESS_TICK = 100;
  constexpr int MAX_FULL_PACKET_SIZE = 89;
  constexpr int MAX_DATA_PACKET_SIZE = 80;

  constexpr int BYTE_SYNC = 0x55;
  constexpr int BYTE_START = 0x7E;
  constexpr int BYTE_END = 0x7E;
  constexpr int BYTE_TYPE_START = 0x01;
  constexpr int BYTE_TYPE_DATA = 0x02;
  constexpr int BYTE_TYPE_END = 0x03;
  constexpr int BYTE_TYPE_NACK = 0x04;
  constexpr int CRC_SIZE = 16;

  //=======================================================
  // Public functions
  //=======================================================

  // Initialize with maximum packet size.
  int initialize(size_t maxPackets);

  // String to send as soon as possible.
  int send(const String& input);

  // Retry from packet x.
  int retry(unsigned char packetIndex);

  // Stop everything you're currently doing. Clear everything.
  int reset(void);
  
  // RTOS task loop
  void handle(void *pv);
}

#endif