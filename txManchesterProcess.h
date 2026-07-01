#ifndef TX_MANCHESTER_PROCESS_H
#define TX_MANCHESTER_PROCESS_H

///////////////////////////////////////////////////////////
// PURPOSE:
// - Manage a queue of byte into a queue of Manchester bits
// - Send a packet as Manchester encoding
// - Utilize TX services to append the bits to send.
// - 1 packet at a time.
///////////////////////////////////////////////////////////

namespace txManchesterProcess {

  //=======================================================
  // Public definitions
  //=======================================================
  constexpr int ERR_NULL_QUEUE = -1;
  constexpr int ERR_NULL_SEMAPHORE = -2;
  constexpr int ERR_BUSY = -3;
  constexpr int ERR_BUFFER_OVERFLOW = -4;

  constexpr int PROCESS_TICK = 1;

  //=======================================================
  // Public functions
  //=======================================================

  // Initialize with maximum packet size.
  int initialize(unsigned char bufferSizeBytes);

  // Add bytes to send as soon as possible.
  int set(const unsigned char* packet, unsigned char byteCount);

  // How many more bits fit in the RX buffer?
  int getRemainingSpace(void);

  // Stop everything you're currently doing.
  int reset(void);
  
  // RTOS task loop
  void handle(void *pv);
}

#endif