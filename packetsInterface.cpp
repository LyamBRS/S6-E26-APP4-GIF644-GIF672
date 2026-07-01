#include "packetsInterface.h"
#include "Arduino.h"

//=========================================================
// Private variables
//=========================================================

//=========================================================
// Private function
//=========================================================

//=========================================================
// Public definitions
//=========================================================

namespace packetsInterface
{
  int calculatePacketCRC(unsigned char* packet, unsigned char packetSize, unsigned char* crcLow, unsigned char* crcHigh)
  {
    if (packet == nullptr || crcLow == nullptr || crcHigh == nullptr)
    {
      return ERR_PARAMETERS_NULL;
    }

    Serial.println("A packet is created");

    unsigned short crc = 0xFFFF;

    for (unsigned char i = 0; i < packetSize; i++)
    {
      crc ^= packet[i];

      for (unsigned char b = 0; b < 8; b++)
      {
        if (crc & 0x0001)
        {
          crc = (crc >> 1) ^ 0xA001;
        }
        else
        {
          crc >>= 1;
        }
      }
    }

    *crcLow  = (unsigned char)(crc & 0xFF);
    *crcHigh = (unsigned char)((crc >> 8) & 0xFF);
    return 0;
  }

  int createStartPacket(unsigned char* packet, unsigned char totalPacketAmount)
  {
    //--------------------------------------------------
    // Header
    //--------------------------------------------------
    packet[INDEX_SYNC] = BYTE_SYNC;
    packet[INDEX_START] = BYTE_START;
    packet[INDEX_TYPE] = BYTE_TYPE_START;
    packet[INDEX_SEQUENCE_NUMBER] = 0x00;
    packet[INDEX_CHARGE_LENGTH] = 0x00;
    packet[INDEX_DYNAMIC_VOLUME] = totalPacketAmount;

    //--------------------------------------------------
    // CRC
    //--------------------------------------------------
    unsigned char crcLow = 0;
    unsigned char crcHigh = 0;
    int result = calculatePacketCRC(packet, 5, &crcLow, &crcHigh);
    if (result != 0) {
      Serial.printf("ERR: packetsInterface: createStartPacket: calculatePacketCRC: %i", result);
      return result;
    }

    packet[6] = crcHigh;
    packet[7] = crcLow;

    //--------------------------------------------------
    // END sequencee
    //--------------------------------------------------
    packet[8] = BYTE_END;
    return 0;
  }

  int createEndPacket(unsigned char* packet, unsigned char sequenceNumber)
  {
    if (packet == nullptr)
    {
      return ERR_PARAMETERS_NULL;
    }

    //--------------------------------------------------
    // Header
    //--------------------------------------------------
    packet[INDEX_SYNC] = BYTE_SYNC;
    packet[INDEX_START] = BYTE_START;
    packet[INDEX_TYPE] = BYTE_TYPE_END;
    packet[INDEX_SEQUENCE_NUMBER] = sequenceNumber;
    packet[INDEX_CHARGE_LENGTH] = 0x00;
    packet[INDEX_DYNAMIC_VOLUME] = 0x00;

    //--------------------------------------------------
    // CRC
    //--------------------------------------------------
    unsigned char crcLow = 0;
    unsigned char crcHigh = 0;
    int result = calculatePacketCRC(packet, 5, &crcLow, &crcHigh);
    if (result != 0) {
      Serial.printf("ERR: packetsInterface: createEndPacket: calculatePacketCRC: %i", result);
      return result;
    }
    packet[6] = crcHigh;
    packet[7] = crcLow;

    //--------------------------------------------------
    // END sequencee
    //--------------------------------------------------
    packet[8] = BYTE_END;
    return 0;
  }
  
  int createRetryPacket(unsigned char* packet, unsigned char sequenceNumber)
  {
    if (packet == nullptr)
    {
      return ERR_PARAMETERS_NULL;
    }

    //--------------------------------------------------
    // Header
    //--------------------------------------------------
    packet[INDEX_SYNC] = BYTE_SYNC;
    packet[INDEX_START] = BYTE_START;
    packet[INDEX_TYPE] = BYTE_TYPE_NACK;
    packet[INDEX_SEQUENCE_NUMBER] = 0x00;
    packet[INDEX_CHARGE_LENGTH] = 0x00;
    packet[INDEX_DYNAMIC_VOLUME] = sequenceNumber;

    //--------------------------------------------------
    // CRC
    //--------------------------------------------------
    unsigned char crcLow = 0;
    unsigned char crcHigh = 0;
    int result = calculatePacketCRC(packet, 5, &crcLow, &crcHigh);
    if (result != 0) {
      Serial.printf("ERR: packetsInterface: createEndPacket: calculatePacketCRC: %i", result);
      return result;
    }
    packet[6] = crcHigh;
    packet[7] = crcLow;

    //--------------------------------------------------
    // END sequencee
    //--------------------------------------------------
    packet[8] = BYTE_END;
    return 0;
  }

  int createDataPacket(unsigned char* packet, unsigned char* packetSize, String &data, unsigned char sequenceNumber)
  {
    if (packet == nullptr || packetSize == nullptr)
    {
      return ERR_PARAMETERS_NULL;
    }

    //--------------------------------------------------
    // Header
    //--------------------------------------------------
    packet[INDEX_SYNC] = BYTE_SYNC;
    packet[INDEX_START] = BYTE_START;
    packet[INDEX_TYPE] = BYTE_TYPE_DATA;
    packet[INDEX_SEQUENCE_NUMBER] = sequenceNumber;
    packet[INDEX_DYNAMIC_VOLUME] = 0x00;

    unsigned char payloadSize = data.length();
    if (payloadSize > MAX_DATA_PACKET_SIZE)
    {
      payloadSize = MAX_DATA_PACKET_SIZE;
    }
    packet[INDEX_CHARGE_LENGTH] = payloadSize;

    //--------------------------------------------------
    // Payload handling
    //--------------------------------------------------

    for (unsigned char i = 0; i < payloadSize; i++)
    {
      packet[5 + i] = (unsigned char)data[i];
    }

    //--------------------------------------------------
    // String handling
    //--------------------------------------------------
    data.remove(0, payloadSize);

    //--------------------------------------------------
    // Final packet size = header + payload + CRC + END
    //--------------------------------------------------
    *packetSize = 5 + payloadSize + 3; // CRC(2) + END(1)

    //--------------------------------------------------
    // CRC
    //--------------------------------------------------
    unsigned char crcLow = 0;
    unsigned char crcHigh = 0;

    int result = calculatePacketCRC(packet, 5 + payloadSize, &crcLow, &crcHigh);
    if (result != 0)
    {
        Serial.printf("ERR: packetsInterface: createDataPacket: calculatePacketCRC: %i", result);
        return result;
    }
    packet[5 + payloadSize]     = crcHigh;
    packet[5 + payloadSize + 1] = crcLow;

    //--------------------------------------------------
    // END byte
    //--------------------------------------------------
    packet[5 + payloadSize + 2] = BYTE_END;
    return 0;
  }
}