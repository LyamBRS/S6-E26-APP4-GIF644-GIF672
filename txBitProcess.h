#ifndef TX_BIT_PROCESS_H
#define TX_BIT_PROCESS_H

///////////////////////////////////////////////////////////
// PURPOSE:
// - Manage a queue of bits to send on a specified GPIO
// - Set the GPIO every task period
// - Work with every protocols that need to schedule bits
///////////////////////////////////////////////////////////

namespace txBitProcess {

  //=======================================================
  // Public definitions
  //=======================================================
  constexpr int ERR_NULL_QUEUE = -1;
  constexpr int ERR_NULL_SEMAPHORE = -2;
  constexpr int ERR_AMOUNT_TOO_HIGH = -3;
  constexpr int ERR_QUEUE_FULL = -4;

  constexpr int PROCESS_TICK = 10;

  //=======================================================
  // Public functions
  //=======================================================
  int initialize(unsigned char buffer_size, int tx_pin);

  // Add an amount of bits to send as soon as possible.
  int append(unsigned char bits, unsigned char amount);

  // How many more bits fit in the RX buffer?
  int getRemainingSpace(void);

  // Stop everything you're currently doing.
  int clearBuffer(void);
  
  // RTOS task loop
  void handle(void *pv);
}

#endif