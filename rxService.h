#ifndef RX_SERVICE_H
#define RX_SERVICE_H

#include "Arduino.h"

///////////////////////////////////////////////////////////
// PURPOSE:
// - Consume validated packets from rxPacketsProcess.
// - Rebuild complete messages from start/data/end frames.
// - Expose completed messages and retry requests.
///////////////////////////////////////////////////////////

namespace rxService {

	//=======================================================
	// Public definitions
	//=======================================================
	constexpr int ERR_NULL_SEMAPHORE = -1;
	constexpr int ERR_NULL_QUEUE = -2;
	constexpr int ERR_ALREADY_INITIALIZED = -3;
	constexpr int ERR_PARAMETERS_NULL = -4;
	constexpr int ERR_EMPTY_QUEUE = -5;
	constexpr int ERR_MESSAGE_TOO_BIG = -6;
	constexpr int ERR_INVALID_PACKET_SEQUENCE = -7;
	constexpr int ERR_EMPTY_RETRY_QUEUE = -8;

	constexpr int PROCESS_TICK = 10;
	constexpr unsigned short MAX_MESSAGE_SIZE = 1024;

	//=======================================================
	// Public functions
	//=======================================================
	int initialize(size_t maxMessages = 4);

	bool available(void);
	int read(unsigned char* message, unsigned short* messageSize);

	bool retryRequested(void);
	int readRetryRequest(unsigned char* packetIndex);

	void handle(void *pv);
}

#endif
