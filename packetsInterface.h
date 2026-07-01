#ifndef PACKET_INTERFACE_H
#define PACKET_INTERFACE_H

#include "Arduino.h"
///////////////////////////////////////////////////////////
// PURPOSE:
///////////////////////////////////////////////////////////

namespace packetsInterface {

  //=======================================================
  // Public definitions
  //=======================================================
  constexpr int ERR_TOO_BIG = -1;
  constexpr int ERR_PARAMETERS_NULL = -2;
  constexpr int ERR_INVALID_MANCHESTER = -3;

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

  constexpr int INDEX_SYNC = 0;
  constexpr int INDEX_START = 1;
  constexpr int INDEX_TYPE = 2;
  constexpr int INDEX_SEQUENCE_NUMBER = 3;
  constexpr int INDEX_CHARGE_LENGTH = 4;
  constexpr int INDEX_DYNAMIC_VOLUME = 5;

  //=======================================================
  // Public functions
  //=======================================================
  int calculatePacketCRC(unsigned char* packet, unsigned char packetSize, unsigned char* crcLow, unsigned char* crcHigh);
  int createStartPacket(unsigned char* packet, unsigned char totalPacketAmount);
  int createEndPacket(unsigned char* packet, unsigned char sequenceNumber);
  int createRetryPacket(unsigned char* packet, unsigned char sequenceNumber);

  // Automatically removes the string parts that were used to create the packet!
  int createDataPacket(unsigned char* packet, unsigned char* packetSize, String &data, unsigned char sequenceNumber);
}

#endif