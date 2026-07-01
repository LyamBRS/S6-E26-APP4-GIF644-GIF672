#include "txPacketsProcess.h"
#include "txManchesterProcess.h"
#include <Arduino.h>

//=========================================================
// Private variables
//=========================================================

static unsigned char** packets = nullptr;
static size_t* packetSizes = nullptr;

static size_t packetCount = 0;
static size_t txIndex = 0;

// Synchronization
static SemaphoreHandle_t readySemaphore = nullptr;
//static SemaphoreHandle_t txLock = nullptr;

static volatile bool aborted = false;
static bool transmitting = false;

//=========================================================
// Private function
//=========================================================
static void manageQueue()
{
  while (txIndex < packetCount)
  {
    if (aborted) return;
    if (txManchesterProcess::ready()) {
      unsigned char* packet = packets[txIndex];
      size_t size = packetSizes[txIndex];

      int result = txManchesterProcess::set(packet, size);
      
      if (result == 0)
      {
        txIndex++;
      }
      else if (result != txManchesterProcess::ERR_BUSY)
      {
        Serial.printf("ERR: txPacketsProcess: manageQueue: txManchesterProcess::set: %i\n", result);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

static int calculatePacketCRC(unsigned char* packet, unsigned char packetSize, unsigned char* crcLow, unsigned char* crcHigh)
{
  if (packet == nullptr || crcLow == nullptr || crcHigh == nullptr)
  {
    return txPacketsProcess::ERR_PARAMETERS_NULL;
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

  //*crcLow  = (unsigned char)(crc & 0xFF);
  //*crcHigh = (unsigned char)((crc >> 8) & 0xFF);
  *crcLow = 0xFF;
  *crcHigh = 0xFF;

  return 0;
}

static int createStartPacket(unsigned char* packet, unsigned char totalPacketAmount)
{
  //--------------------------------------------------
  // Header
  //--------------------------------------------------
  packet[0] = txPacketsProcess::BYTE_SYNC;
  packet[1] = txPacketsProcess::BYTE_START;
  packet[2] = txPacketsProcess::BYTE_TYPE_START;
  //packet[3] = 0x00;
  //packet[4] = 0x00;
  packet[3] = 0xFF;
  packet[4] = 0xFF;
  packet[5] = 0xFF;//totalPacketAmount;

  //--------------------------------------------------
  // CRC
  //--------------------------------------------------
  unsigned char crcLow = 0;
  unsigned char crcHigh = 0;
  int result = calculatePacketCRC(packet, 5, &crcLow, &crcHigh);
  if (result != 0) {
    Serial.printf("ERR: txPacketsProcess: createStartPacket: calculatePacketCRC: %i", result);
    return result;
  }

  packet[6] = crcHigh;
  packet[7] = crcLow;

  //--------------------------------------------------
  // END sequencee
  //--------------------------------------------------
  packet[8] = txPacketsProcess::BYTE_END;
  return 0;
}

static int createEndPacket(unsigned char* packet, unsigned char sequenceNumber)
{
  if (packet == nullptr)
  {
    return txPacketsProcess::ERR_PARAMETERS_NULL;
  }

  //--------------------------------------------------
  // Header
  //--------------------------------------------------
  packet[0] = txPacketsProcess::BYTE_SYNC;
  packet[1] = txPacketsProcess::BYTE_START;
  packet[2] = txPacketsProcess::BYTE_TYPE_END;
  packet[3] = 0xFF;//sequenceNumber;
  //packet[4] = 0x00;
  //packet[5] = 0x00;
  packet[4] = 0xFF;
  packet[5] = 0xFF;

  //--------------------------------------------------
  // CRC
  //--------------------------------------------------
  unsigned char crcLow = 0;
  unsigned char crcHigh = 0;
  int result = calculatePacketCRC(packet, 5, &crcLow, &crcHigh);
  if (result != 0) {
    Serial.printf("ERR: txPacketsProcess: createEndPacket: calculatePacketCRC: %i", result);
    return result;
  }
  packet[6] = crcHigh;
  packet[7] = crcLow;

  //--------------------------------------------------
  // END sequencee
  //--------------------------------------------------
  packet[8] = txPacketsProcess::BYTE_END;
  return 0;
}

static int createDataPacket(unsigned char* packet, unsigned char* packetSize, String &data, unsigned char sequenceNumber)
{
  if (packet == nullptr || packetSize == nullptr)
  {
    return txPacketsProcess::ERR_PARAMETERS_NULL;
  }

  //--------------------------------------------------
  // Header
  //--------------------------------------------------
  packet[0] = txPacketsProcess::BYTE_SYNC;
  packet[1] = txPacketsProcess::BYTE_START;
  packet[2] = txPacketsProcess::BYTE_TYPE_DATA;
  //packet[3] = sequenceNumber;

  unsigned char payloadSize = data.length();
  if (payloadSize > txPacketsProcess::MAX_DATA_PACKET_SIZE)
  {
    payloadSize = txPacketsProcess::MAX_DATA_PACKET_SIZE;
  }
  //packet[4] = payloadSize;
  packet[3] = 0xFF;
  packet[4] = 0xFF;

  //--------------------------------------------------
  // Payload handling
  //--------------------------------------------------

  for (unsigned char i = 0; i < payloadSize; i++)
  {
    packet[5 + i] = 0xFF;//(unsigned char)data[i];
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
      Serial.printf("ERR: txPacketsProcess: createDataPacket: calculatePacketCRC: %i", result);
      return result;
  }
  packet[5 + payloadSize]     = crcHigh;
  packet[5 + payloadSize + 1] = crcLow;

  //--------------------------------------------------
  // END byte
  //--------------------------------------------------
  packet[5 + payloadSize + 2] = txPacketsProcess::BYTE_END;
  return 0;
}

//=========================================================
// Public definitions
//=========================================================

namespace txPacketsProcess
{

  int initialize(size_t maxPackets)
  {
    if (packets != nullptr || packetSizes != nullptr)
    {
      return ERR_ALREADY_INITIALIZED;
    }

    //--------------------------------------------------
    // Packet saving definitions
    //--------------------------------------------------
    packets = new unsigned char*[maxPackets];
    if (packets == nullptr)
    {
      return ERR_OUT_OF_MEMORY;
    }

    packetSizes = new size_t[maxPackets];
    if (packetSizes == nullptr)
    {
      delete[] packets;
      packets = nullptr;
      return ERR_OUT_OF_MEMORY;
    }

    //--------------------------------------------------
    // Reset runtime state
    //--------------------------------------------------
    packetCount = 0;
    txIndex = 0;
    aborted = false;
    transmitting = false;

    //--------------------------------------------------
    // RTOS
    //--------------------------------------------------
    readySemaphore = xSemaphoreCreateBinary();
    if (readySemaphore == nullptr)
    {
      delete[] packets;
      delete[] packetSizes;
      packets = nullptr;
      packetSizes = nullptr;
      return ERR_NULL_SEMAPHORE;
    }

    xSemaphoreGive(readySemaphore);

    return 0;
  }

  int send(const String& input)
  {
    if (transmitting) {
      return ERR_BUSY;
    }
    transmitting = true;
    aborted = false;

    //--------------------------------------------------
    // Estimated packet count
    //--------------------------------------------------
    String data = input;

    const size_t MAX_PACKETS_ESTIMATE = (data.length() / MAX_DATA_PACKET_SIZE) + 3; // start + end + safety

    //--------------------------------------------------
    // Storage allocations
    //--------------------------------------------------
    packets = new unsigned char*[MAX_PACKETS_ESTIMATE];
    packetSizes = new size_t[MAX_PACKETS_ESTIMATE];
    packetCount = 0;
    txIndex = 0;

    unsigned char sequenceNumber = 0;

    //--------------------------------------------------
    // Helper: store packet
    //--------------------------------------------------
    auto storePacket = [&](unsigned char* packet, size_t size)
    {
        packets[packetCount] = packet;
        packetSizes[packetCount] = size;
        packetCount++;
    };

    //--------------------------------------------------
    // START PACKET
    //--------------------------------------------------
    {
        unsigned char* packet = new unsigned char[9];
        if (!packet) return ERR_OUT_OF_MEMORY;

        int result = createStartPacket(
            packet,
            (data.length() / MAX_DATA_PACKET_SIZE) + 1
        );

        if (result != 0)
        {
            delete[] packet;
            transmitting = false;
            Serial.printf("ERR: txPacketsProcess: send: createStartPacket: %i\n", result);
            return result;
        }

        storePacket(packet, 9);
    }

    //--------------------------------------------------
    // DATA PACKETS
    //--------------------------------------------------
    while (data.length() > 0)
    {
        unsigned char* packet = new unsigned char[MAX_FULL_PACKET_SIZE];
        if (!packet) return ERR_OUT_OF_MEMORY;

        unsigned char size = 0;

        int result = createDataPacket(
            packet,
            &size,
            data,
            sequenceNumber++
        );

        if (result != 0)
        {
            delete[] packet;
            transmitting = false;
            Serial.printf("ERR: txPacketsProcess: send: createDataPacket: %i\n", result);
            return result;
        }

        storePacket(packet, size);
    }

    //--------------------------------------------------
    // END PACKET
    //--------------------------------------------------
    {
        unsigned char* packet = new unsigned char[9];
        if (!packet) return ERR_OUT_OF_MEMORY;

        int result = createEndPacket(packet, sequenceNumber);
        if (result != 0)
        {
            delete[] packet;
            transmitting = false;
            Serial.printf("ERR: txPacketsProcess: send: createEndPacket: %i\n", result);
            return result;
        }

        storePacket(packet, 9);
    }


    return 0;
  }

  int retry(unsigned char packetIndex)
  {
    if (packetIndex < 0 || packetIndex >= packetCount)
    {
      return ERR_INVALID_INDEX;
    }

    if (!transmitting) {
      return ERR_NOT_TRANSMITTING;
    }

    txIndex = packetIndex;
    return 0;
  }

  int reset()
  {
    for (size_t i = 0; i < packetCount; i++)
    {
      delete[] packets[i];
    }

    delete[] packets;
    delete[] packetSizes;

    packets = nullptr;
    packetSizes = nullptr;
    packetCount = 0;
    txIndex = 0;
    transmitting = false;
    return 0;
  }

  void handle(void* pv)
  {
    while (true)
    {
      vTaskDelay(pdMS_TO_TICKS(PROCESS_TICK));
      if (!aborted)
      {
        if (packetCount > 0 && txIndex < packetCount)
        {
          manageQueue();
        }
        else if (packetCount > 0 && txIndex >= packetCount)
        {
          transmitting = false;
        }

      }
    }
  }
}