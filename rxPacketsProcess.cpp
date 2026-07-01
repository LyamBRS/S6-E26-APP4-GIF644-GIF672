#include "rxPacketsProcess.h"
#include "packetsInterface.h"
#include "Arduino.h"

//=========================================================
// Private variables
//=========================================================
struct CompletedPacket
{
	unsigned char size;
	unsigned char data[packetsInterface::MAX_FULL_PACKET_SIZE];
};

static QueueHandle_t byteQueue = nullptr;
static QueueHandle_t packetQueue = nullptr;
static SemaphoreHandle_t readySemaphore = nullptr;

static bool collecting = false;
static unsigned char packetBuffer[packetsInterface::MAX_FULL_PACKET_SIZE] = {0};
static unsigned char packetIndex = 0;
static unsigned char expectedSize = 0;

//=========================================================
// Private function
//=========================================================
static void resetCollector()
{
	collecting = false;
	packetIndex = 0;
	expectedSize = 0;
}

static unsigned char expectedPacketSizeFromHeader(void)
{
	unsigned char type = packetBuffer[packetsInterface::INDEX_TYPE];

	if (type == packetsInterface::BYTE_TYPE_DATA)
	{
		unsigned char payloadSize = packetBuffer[packetsInterface::INDEX_CHARGE_LENGTH];
		if (payloadSize > packetsInterface::MAX_DATA_PACKET_SIZE)
		{
			return 0;
		}

		unsigned char totalSize = (unsigned char)(5 + payloadSize + 3);
		return totalSize;
	}

	if (type == packetsInterface::BYTE_TYPE_START ||
			type == packetsInterface::BYTE_TYPE_END ||
			type == packetsInterface::BYTE_TYPE_NACK)
	{
		return 9;
	}

	return 0;
}

static int enqueueCompletedPacket(void)
{
	if (packetQueue == nullptr)
	{
		return rxPacketsProcess::ERR_NULL_QUEUE;
	}

	CompletedPacket completedPacket;
	completedPacket.size = packetIndex;

	for (unsigned char i = 0; i < packetIndex; i++)
	{
		completedPacket.data[i] = packetBuffer[i];
	}

	if (xQueueSend(packetQueue, &completedPacket, 0) != pdPASS)
	{
		return rxPacketsProcess::ERR_QUEUE_FULL;
	}

	return 0;
}

static void processBytes(void)
{
	unsigned char byte;

	if (xQueueReceive(byteQueue, &byte, portMAX_DELAY) != pdPASS)
	{
		return;
	}

	if (!collecting)
	{
		if (byte == packetsInterface::BYTE_SYNC)
		{
			collecting = true;
			packetBuffer[0] = byte;
			packetIndex = 1;
		}
		return;
	}

	if (packetIndex >= packetsInterface::MAX_FULL_PACKET_SIZE)
	{
		resetCollector();
		return;
	}

	packetBuffer[packetIndex++] = byte;

	if (packetIndex == 2 && byte != packetsInterface::BYTE_START)
	{
		resetCollector();
		return;
	}

	if (packetIndex == 5)
	{
		expectedSize = expectedPacketSizeFromHeader();
		if (expectedSize == 0 || expectedSize > packetsInterface::MAX_FULL_PACKET_SIZE)
		{
			resetCollector();
			return;
		}
	}

	if (expectedSize != 0 && packetIndex == expectedSize)
	{
		unsigned char crcLow = 0;
		unsigned char crcHigh = 0;
		unsigned char crcSize = 5;

		if (packetBuffer[packetsInterface::INDEX_TYPE] == packetsInterface::BYTE_TYPE_DATA)
		{
			unsigned char payloadSize = packetBuffer[packetsInterface::INDEX_CHARGE_LENGTH];
			crcSize = (unsigned char)(5 + payloadSize);
		}

		int result = packetsInterface::calculatePacketCRC(packetBuffer, crcSize, &crcLow, &crcHigh);
		if (result == 0)
		{
			if (packetBuffer[packetIndex - 1] == packetsInterface::BYTE_END &&
					packetBuffer[packetIndex - 3] == crcHigh &&
					packetBuffer[packetIndex - 2] == crcLow)
			{
				result = enqueueCompletedPacket();
				if (result != 0)
				{
					Serial.printf("ERR: rxPacketsProcess::processBytes: enqueueCompletedPacket: %i\n", result);
				}
			}
			else
			{
				Serial.println("ERR: rxPacketsProcess::processBytes: invalid packet");
			}
		}
		else
		{
			Serial.printf("ERR: rxPacketsProcess::processBytes: calculatePacketCRC: %i\n", result);
		}

		resetCollector();
	}
}

//=========================================================
// Public definitions
//=========================================================

namespace rxPacketsProcess
{
	int initialize(unsigned char bufferSizeBytes)
	{
		if (bufferSizeBytes == 0)
		{
			return ERR_BUFFER_OVERFLOW;
		}

		byteQueue = xQueueCreate(bufferSizeBytes, sizeof(unsigned char));
		if (byteQueue == nullptr)
		{
			return ERR_NULL_QUEUE;
		}

		packetQueue = xQueueCreate(bufferSizeBytes, sizeof(CompletedPacket));
		if (packetQueue == nullptr)
		{
			return ERR_NULL_QUEUE;
		}

		readySemaphore = xSemaphoreCreateBinary();
		if (readySemaphore == nullptr)
		{
			return ERR_NULL_SEMAPHORE;
		}

		collecting = false;
		packetIndex = 0;
		expectedSize = 0;

		xSemaphoreGive(readySemaphore);

		return 0;
	}

	int append(unsigned char byte)
	{
		if (byteQueue == nullptr)
		{
			return ERR_NULL_QUEUE;
		}

		if (xQueueSend(byteQueue, &byte, 0) != pdPASS)
		{
			return ERR_QUEUE_FULL;
		}

		return 0;
	}

	int getRemainingSpace()
	{
		if (byteQueue == nullptr)
		{
			return ERR_NULL_QUEUE;
		}

		return uxQueueSpacesAvailable(byteQueue);
	}

	int clearBuffer()
	{
		if (byteQueue == nullptr || packetQueue == nullptr)
		{
			return ERR_NULL_QUEUE;
		}

		xQueueReset(byteQueue);
		xQueueReset(packetQueue);
		resetCollector();

		return 0;
	}

	bool available(void)
	{
		if (packetQueue == nullptr)
		{
			return false;
		}

		return uxQueueMessagesWaiting(packetQueue) > 0;
	}

	int read(unsigned char* packet, unsigned char* packetSize)
	{
		if (packet == nullptr || packetSize == nullptr)
		{
			return ERR_PARAMETERS_NULL;
		}

		if (packetQueue == nullptr)
		{
			return ERR_NULL_QUEUE;
		}

		CompletedPacket completedPacket;
		if (xQueueReceive(packetQueue, &completedPacket, 0) != pdPASS)
		{
			return ERR_EMPTY_QUEUE;
		}

		*packetSize = completedPacket.size;
		for (unsigned char i = 0; i < completedPacket.size; i++)
		{
			packet[i] = completedPacket.data[i];
		}

		return 0;
	}

	void handle(void *pv)
	{
		xSemaphoreTake(readySemaphore, portMAX_DELAY);

		while (true)
		{
			processBytes();
		}
	}
}
