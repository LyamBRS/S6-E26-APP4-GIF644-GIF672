#include "rxBitprocess.h"
#include "rxManchesterProcess.h"
#include "Arduino.h"

//=========================================================
// Private variables
//=========================================================
static SemaphoreHandle_t readySemaphore = nullptr;
static int rxPin = -1;

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

		readySemaphore = xSemaphoreCreateBinary();
		if (readySemaphore == nullptr)
		{
			return ERR_NULL_SEMAPHORE;
		}

		xSemaphoreGive(readySemaphore);

		return 0;
	}

	void handle(void *pv)
	{
		xSemaphoreTake(readySemaphore, portMAX_DELAY);

		while (true)
		{
			bool bit = digitalRead(rxPin) == HIGH;

			int result = rxManchesterProcess::append(bit);
			if (result != 0 && result != rxManchesterProcess::ERR_QUEUE_FULL)
			{
				Serial.printf("ERR: rxBitProcess::handle: rxManchesterProcess::append: %i\n", result);
			}

			delayMicroseconds(PROCESS_SPEED_US);
		}
	}
}
