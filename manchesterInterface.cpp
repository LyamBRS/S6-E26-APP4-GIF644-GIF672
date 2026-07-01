#include "manchesterInterface.h"

//=========================================================
// Private variables
//=========================================================

//=========================================================
// Private function
//=========================================================

//=========================================================
// Public definitions
//=========================================================

namespace manchesterInterface
{
  // TESTED
  int fromUnsignedChar(unsigned char toConvert, unsigned char* outLow, unsigned char* outHigh)
  {
    if (!outLow || !outHigh)
    {
      return ERR_PARAMETERS_NULL;
    }

    unsigned char low = 0;
    unsigned char high = 0;

    for (int i = 0; i < 8; i++)
    {
        unsigned char bit = (toConvert >> (7 - i)) & 0x01;

        unsigned char manLow = bit == 1;
        unsigned char manHigh = bit == 0;

        // write into output stream (16 bits total)
        int outIndex = i * 2;
        if (outIndex < 8)
        {
            high |= (manLow << (7 - outIndex));
            high |= (manHigh << (7 - outIndex - 1));
        }
        else
        {
            int shift = 15 - outIndex;
            low |= (manLow << shift);
            low |= (manHigh << (shift - 1));
        }
    }

    *outLow = low;
    *outHigh = high;

    //Serial.println("CONVERSION");
    //Serial.print("this: ");
    //printBits(toConvert);
    //Serial.print("\nResulted in: ");
    //printBits(high);
    //printBits(low);
    //Serial.println("\n");

    return 0;
  }

  int toUnsignedChar(unsigned char low, unsigned char high, unsigned char* result)
  {
    if (!result) { return ERR_PARAMETERS_NULL; }
    unsigned char output = 0;
    for (int i = 0; i < 8; i++)
    {
      int outIndex = i * 2;
      unsigned char manLow, manHigh;
      if (outIndex < 8)
      {
        manLow  = (high >> (7 - outIndex)) & 0x01;
        manHigh = (high >> (7 - outIndex - 1)) & 0x01;
      }
      else
      {
        int shift = 15 - outIndex;
        manLow  = (low >> shift) & 0x01;
        manHigh = (low >> (shift - 1)) & 0x01;
      }

      unsigned char bit;
      if (manLow == 0 && manHigh == 1) bit = 0;
      else if (manLow == 1 && manHigh == 0) bit = 1;
      else return ERR_INVALID_MANCHESTER;

      output |= (bit << (7 - i));
    }
    *result = output;
    return 0;
  }
}