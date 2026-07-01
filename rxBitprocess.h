#ifndef RX_BIT_PROCESS_H
#define RX_BIT_PROCESS_H

///////////////////////////////////////////////////////////
// PURPOSE:
// - Sample a GPIO line and forward the bits to the
//   Manchester decoder.
///////////////////////////////////////////////////////////

namespace rxBitProcess {

	//=======================================================
	// Public definitions
	//=======================================================
	constexpr int ERR_NULL_SEMAPHORE = -1;
	constexpr int ERR_INVALID_PIN = -2;

	constexpr int PROCESS_SPEED_US = 50;

	//=======================================================
	// Public functions
	//=======================================================
	int initialize(int rx_pin);

	// RTOS task loop
	void handle(void *pv);
}

#endif
