#ifndef MANCHESTER_INTERFACE_H
#define MANCHESTER_INTERFACE_H

///////////////////////////////////////////////////////////
// PURPOSE:
// - Transform bits into manchester bits
// - Transform manchester codes into regular bits
///////////////////////////////////////////////////////////

namespace manchesterInterface {

  //=======================================================
  // Public definitions
  //=======================================================
  constexpr int ERR_TOO_BIG = -1;
  constexpr int ERR_PARAMETERS_NULL = -2;
  constexpr int ERR_INVALID_MANCHESTER = -3;

  //=======================================================
  // Public functions
  //=======================================================
  int fromUnsignedChar(unsigned char toConvert, unsigned char* outLow, unsigned char* outHigh);
  int toUnsignedChar(unsigned char low, unsigned char high, unsigned char* result);
}

#endif