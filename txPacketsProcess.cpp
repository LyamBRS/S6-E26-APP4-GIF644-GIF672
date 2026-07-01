#include "txPacketsProcess.h"
#include "txManchesterProcess.h"
#include "packetsInterface.h"
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

static void storePacket(unsigned char* packet, size_t size)
{
  packets[packetCount] = packet;
  packetSizes[packetCount] = size;
  packetCount++;
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

  bool ready(void)
  {
    return !transmitting;
  }

  int sendRetry(unsigned char sequenceNumber)
  {
    if (transmitting) {
      return ERR_BUSY;
    }
    transmitting = true;
    aborted = false;

    unsigned char* packet = new unsigned char[9];
    if (!packet) return ERR_OUT_OF_MEMORY;

    int result = packetsInterface::createRetryPacket(packet, sequenceNumber);
    if (result != 0)
    {
      delete[] packet;
      transmitting = false;
      Serial.printf("ERR: txPacketsProcess: sendRetry: createRetryPacket: %i\n", result);
      return result;
    }

    storePacket(packet, 9);
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

    const size_t MAX_PACKETS_ESTIMATE = (data.length() / packetsInterface::MAX_DATA_PACKET_SIZE) + 3; // start + end + safety

    //--------------------------------------------------
    // Storage allocations
    //--------------------------------------------------
    packets = new unsigned char*[MAX_PACKETS_ESTIMATE];
    packetSizes = new size_t[MAX_PACKETS_ESTIMATE];
    packetCount = 0;
    txIndex = 0;
    unsigned char sequenceNumber = 0;

    //--------------------------------------------------
    // START PACKET
    //--------------------------------------------------
    {
        unsigned char* packet = new unsigned char[9];
        if (!packet) return ERR_OUT_OF_MEMORY;

        int result = packetsInterface::createStartPacket(
            packet,
            (data.length() / packetsInterface::MAX_DATA_PACKET_SIZE) + 1
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
        unsigned char* packet = new unsigned char[packetsInterface::MAX_FULL_PACKET_SIZE];
        if (!packet) return ERR_OUT_OF_MEMORY;

        unsigned char size = 0;

        int result = packetsInterface::createDataPacket(
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

        int result = packetsInterface::createEndPacket(packet, sequenceNumber);
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