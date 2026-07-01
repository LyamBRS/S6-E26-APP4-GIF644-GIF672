#include "rxService.h"
#include "rxPacketsProcess.h"
#include "packetsInterface.h"

//=========================================================
// Private variables
//=========================================================
struct CompletedMessage
{
	unsigned short size;
	unsigned char data[rxService::MAX_MESSAGE_SIZE];
};

static QueueHandle_t messageQueue = nullptr;
static QueueHandle_t retryQueue = nullptr;
static SemaphoreHandle_t readySemaphore = nullptr;

static bool receiving = false;
static bool packetCountKnown = false;
static unsigned char expectedSequence = 0;
static unsigned char expectedPacketAmount = 0;
static unsigned short currentMessageSize = 0;
static unsigned char currentMessage[rxService::MAX_MESSAGE_SIZE] = {0};

//=========================================================
// Private functions
//=========================================================
static void resetCollector()
{
	receiving = false;
	packetCountKnown = false;
	expectedSequence = 0;
	expectedPacketAmount = 0;
	currentMessageSize = 0;
}

static int pushRetryRequest(unsigned char packetIndex)
{
	if (retryQueue == nullptr)
	{
		return rxService::ERR_NULL_QUEUE;
	}

	if (xQueueSend(retryQueue, &packetIndex, 0) != pdPASS)
	{
		return rxService::ERR_NULL_QUEUE;
	}

	return 0;
}

static int enqueueCompletedMessage(void)
{
	if (messageQueue == nullptr)
	{
		return rxService::ERR_NULL_QUEUE;
	}

	CompletedMessage completedMessage;
	completedMessage.size = currentMessageSize;
	for (unsigned short i = 0; i < currentMessageSize; i++)
	{
		completedMessage.data[i] = currentMessage[i];
	}

	if (xQueueSend(messageQueue, &completedMessage, 0) != pdPASS)
	{
		return rxService::ERR_NULL_QUEUE;
	}

	return 0;
}

static int appendPayload(const unsigned char* packet, unsigned char packetSize)
{
	unsigned char payloadSize = packet[packetsInterface::INDEX_CHARGE_LENGTH];
	unsigned char expectedSize = (unsigned char)(5 + payloadSize + 3);
	if (packetSize != expectedSize)
	{
		return rxService::ERR_INVALID_PACKET_SEQUENCE;
	}

	if ((unsigned short)(currentMessageSize + payloadSize) > rxService::MAX_MESSAGE_SIZE)
	{
		return rxService::ERR_MESSAGE_TOO_BIG;
	}

	for (unsigned char i = 0; i < payloadSize; i++)
	{
		currentMessage[currentMessageSize++] = packet[packetsInterface::INDEX_DYNAMIC_VOLUME + i];
	}

	return 0;
}

static int processPacket(const unsigned char* packet, unsigned char packetSize)
{
	if (packet == nullptr || packetSize < 9)
	{
		return rxService::ERR_PARAMETERS_NULL;
	}

	if (packet[packetsInterface::INDEX_SYNC] != packetsInterface::BYTE_SYNC ||
			packet[packetsInterface::INDEX_START] != packetsInterface::BYTE_START ||
			packet[packetSize - 1] != packetsInterface::BYTE_END)
	{
		unsigned char retryIndex = receiving ? expectedSequence : 0;
		pushRetryRequest(retryIndex);
		resetCollector();
		return rxService::ERR_INVALID_PACKET_SEQUENCE;
	}

	unsigned char packetType = packet[packetsInterface::INDEX_TYPE];
	unsigned char sequenceNumber = packet[packetsInterface::INDEX_SEQUENCE_NUMBER];

	if (packetType == packetsInterface::BYTE_TYPE_START)
	{
		resetCollector();
		receiving = true;
		packetCountKnown = true;
		expectedPacketAmount = packet[packetsInterface::INDEX_DYNAMIC_VOLUME];
		return 0;
	}

	if (!receiving)
	{
		pushRetryRequest(0);
		return rxService::ERR_INVALID_PACKET_SEQUENCE;
	}

	if (packetType == packetsInterface::BYTE_TYPE_DATA)
	{
		if (sequenceNumber != expectedSequence)
		{
			pushRetryRequest(expectedSequence);
			resetCollector();
			return rxService::ERR_INVALID_PACKET_SEQUENCE;
		}

		int result = appendPayload(packet, packetSize);
		if (result != 0)
		{
			pushRetryRequest(expectedSequence);
			resetCollector();
			return result;
		}

		expectedSequence++;
		return 0;
	}

	if (packetType == packetsInterface::BYTE_TYPE_END)
	{
		if (sequenceNumber != expectedSequence)
		{
			pushRetryRequest(expectedSequence);
			resetCollector();
			return rxService::ERR_INVALID_PACKET_SEQUENCE;
		}

		if (packetCountKnown && expectedPacketAmount != 0 && expectedSequence != expectedPacketAmount)
		{
			pushRetryRequest(expectedSequence);
			resetCollector();
			return rxService::ERR_INVALID_PACKET_SEQUENCE;
		}

		int result = enqueueCompletedMessage();
		resetCollector();
		return result;
	}

	if (packetType == packetsInterface::BYTE_TYPE_NACK)
	{
		resetCollector();
		return 0;
	}

	pushRetryRequest(expectedSequence);
	resetCollector();
	return rxService::ERR_INVALID_PACKET_SEQUENCE;
}

//=========================================================
// Public definitions
//=========================================================

namespace rxService
{
	int initialize(size_t maxMessages)
	{
		if (messageQueue != nullptr || retryQueue != nullptr || readySemaphore != nullptr)
		{
			return ERR_ALREADY_INITIALIZED;
		}

		messageQueue = xQueueCreate(maxMessages, sizeof(CompletedMessage));
		if (messageQueue == nullptr)
		{
			return ERR_NULL_QUEUE;
		}

		retryQueue = xQueueCreate(maxMessages, sizeof(unsigned char));
		if (retryQueue == nullptr)
		{
			return ERR_NULL_QUEUE;
		}

		readySemaphore = xSemaphoreCreateBinary();
		if (readySemaphore == nullptr)
		{
			return ERR_NULL_SEMAPHORE;
		}

		resetCollector();
		xSemaphoreGive(readySemaphore);

		return 0;
	}

	bool available(void)
	{
		if (messageQueue == nullptr)
		{
			return false;
		}

		return uxQueueMessagesWaiting(messageQueue) > 0;
	}

	int read(unsigned char* message, unsigned short* messageSize)
	{
		if (message == nullptr || messageSize == nullptr)
		{
			return ERR_PARAMETERS_NULL;
		}

		if (messageQueue == nullptr)
		{
			return ERR_NULL_QUEUE;
		}

		CompletedMessage completedMessage;
		if (xQueueReceive(messageQueue, &completedMessage, 0) != pdPASS)
		{
			return ERR_EMPTY_QUEUE;
		}

		*messageSize = completedMessage.size;
		for (unsigned short i = 0; i < completedMessage.size; i++)
		{
			message[i] = completedMessage.data[i];
		}

		return 0;
	}

	bool retryRequested(void)
	{
		if (retryQueue == nullptr)
		{
			return false;
		}

		return uxQueueMessagesWaiting(retryQueue) > 0;
	}

	int readRetryRequest(unsigned char* packetIndex)
	{
		if (packetIndex == nullptr)
		{
			return ERR_PARAMETERS_NULL;
		}

		if (retryQueue == nullptr)
		{
			return ERR_NULL_QUEUE;
		}

		if (xQueueReceive(retryQueue, packetIndex, 0) != pdPASS)
		{
			return ERR_EMPTY_RETRY_QUEUE;
		}

		return 0;
	}

	void handle(void *pv)
	{
		xSemaphoreTake(readySemaphore, portMAX_DELAY);

		while (true)
		{
			if (rxPacketsProcess::available())
			{
				unsigned char packet[packetsInterface::MAX_FULL_PACKET_SIZE] = {0};
				unsigned char packetSize = 0;

				int result = rxPacketsProcess::read(packet, &packetSize);
				if (result != 0)
				{
					Serial.printf("ERR: rxService::handle: rxPacketsProcess::read: %i\n", result);
				}
				else
				{
					result = processPacket(packet, packetSize);
					if (result != 0 && result != ERR_INVALID_PACKET_SEQUENCE)
					{
						Serial.printf("ERR: rxService::handle: processPacket: %i\n", result);
					}
				}
			}

			vTaskDelay(pdMS_TO_TICKS(PROCESS_TICK));
		}
	}
}
