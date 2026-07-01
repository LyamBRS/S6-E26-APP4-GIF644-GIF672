#ifndef RX_MANCHESTER_PROCESS_H
#define RX_MANCHESTER_PROCESS_H

///////////////////////////////////////////////////////////
// PURPOSE:
// - Convert sampled bits into bytes using Manchester
//   decoding.
///////////////////////////////////////////////////////////

namespace rxManchesterProcess {

  //=======================================================
  // Public definitions
  //=======================================================
  constexpr int ERR_NULL_QUEUE = -1;
  constexpr int ERR_NULL_SEMAPHORE = -2;
  constexpr int ERR_QUEUE_FULL = -3;

  constexpr int PROCESS_SPEED_US = 10;

  //=======================================================
  // Public functions
  //=======================================================
  int initialize(unsigned char bufferSizeBits);

  int append(bool bit);

  int getRemainingSpace(void);

  int clearBuffer(void);

  // RTOS task loop
  void handle(void *pv);
}

#endif