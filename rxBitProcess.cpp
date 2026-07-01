#include "rxBitprocess.h"
#include "rxManchesterProcess.h"
#include "Arduino.h"
#include "esp_timer.h"

//=========================================================
// Private variables
//=========================================================
struct EdgeEvent
{
	uint32_t timestampUs;
};

static QueueHandle_t edgeQueue = nullptr;
static SemaphoreHandle_t readySemaphore = nullptr;
static int rxPin = -1;
static volatile bool haveLastEdge = false;
static volatile uint32_t lastEdgeTimestampUs = 0;
static bool currentBit = false;

//=========================================================
// Private functions
//=========================================================
static void IRAM_ATTR onFallingEdge(void)
{
	if (edgeQueue == nullptr)
	{
		return;
	}

	EdgeEvent event;
	event.timestampUs = (uint32_t)esp_timer_get_time();

	BaseType_t taskWoken = pdFALSE;
	xQueueSendFromISR(edgeQueue, &event, &taskWoken);
	if (taskWoken == pdTRUE)
	{
		portYIELD_FROM_ISR();
	}
}

static void processEdge(const EdgeEvent &event)
{
	if (!haveLastEdge)
	{
		haveLastEdge = true;
		lastEdgeTimestampUs = event.timestampUs;
		return;
	}

	uint32_t elapsedUs = event.timestampUs - lastEdgeTimestampUs;
	lastEdgeTimestampUs = event.timestampUs;

	if (elapsedUs >= rxBitProcess::EDGE_CHANGE_THRESHOLD_US)
	{
		currentBit = !currentBit;
	}

	int result = rxManchesterProcess::append(currentBit);
	if (result != 0 && result != rxManchesterProcess::ERR_QUEUE_FULL)
	{
		Serial.printf("ERR: rxBitProcess::processEdge: rxManchesterProcess::append: %i\n", result);
	}
}

//=========================================================
// Public definitions
//=========================================================

namespace rxBitProcess
{
	int initialize(int rx_pin)
	{
		if (rx_pin < 0)
		{
			return ERR_INVALID_PIN;
		}

		rxPin = rx_pin;
		pinMode(rxPin, INPUT);
		currentBit = digitalRead(rxPin) == HIGH;

		edgeQueue = xQueueCreate(EDGE_QUEUE_SIZE, sizeof(EdgeEvent));
		if (edgeQueue == nullptr)
		{
			return ERR_NULL_QUEUE;
		}

		readySemaphore = xSemaphoreCreateBinary();
		if (readySemaphore == nullptr)
		{
			return ERR_NULL_SEMAPHORE;
		}

		attachInterrupt(digitalPinToInterrupt(rxPin), onFallingEdge, FALLING);

		xSemaphoreGive(readySemaphore);

		return 0;
	}

	void handle(void *pv)
	{
		xSemaphoreTake(readySemaphore, portMAX_DELAY);

		while (true)
		{
			EdgeEvent event;
			if (xQueueReceive(edgeQueue, &event, portMAX_DELAY) != pdPASS)
			{
				continue;
			}

			processEdge(event);
		}
	}
}
