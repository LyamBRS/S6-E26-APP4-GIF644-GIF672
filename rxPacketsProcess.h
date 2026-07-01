#ifndef RX_PACKETS_PROCESS_H
#define RX_PACKETS_PROCESS_H

#include "Arduino.h"

///////////////////////////////////////////////////////////
// PURPOSE:
// - Receive Manchester-decoded bytes.
// - Reconstruct full packets and verify their CRC.
///////////////////////////////////////////////////////////

namespace rxPacketsProcess {

	//=======================================================
	// Public definitions
	//=======================================================
	constexpr int ERR_NULL_QUEUE = -1;
	constexpr int ERR_NULL_SEMAPHORE = -2;
	constexpr int ERR_QUEUE_FULL = -3;
	constexpr int ERR_BUFFER_OVERFLOW = -4;
	constexpr int ERR_EMPTY_QUEUE = -5;
	constexpr int ERR_PARAMETERS_NULL = -6;
	constexpr int ERR_INVALID_PACKET = -7;

	constexpr int PROCESS_TICK = 1;

	//=======================================================
	// Public functions
	//=======================================================
	int initialize(unsigned char bufferSizeBytes);

	int append(unsigned char byte);

	int getRemainingSpace(void);

	int clearBuffer(void);

	bool available(void);

	int read(unsigned char* packet, unsigned char* packetSize);

	// RTOS task loop
	void handle(void *pv);
}

#endif
