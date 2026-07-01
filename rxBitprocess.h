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
	constexpr int ERR_NULL_QUEUE = -3;

	constexpr unsigned long EDGE_CHANGE_THRESHOLD_US = 150;
	constexpr unsigned long EDGE_QUEUE_SIZE = 32;

	//=======================================================
	// Public functions
	//=======================================================
	int initialize(int rx_pin);

	// RTOS task loop
	void handle(void *pv);
}

#endif
