/******************************************************************************
**              Copyright (C) 2019 Laird Connectivity
**
** Project:     Pinnacle 100
**
** Module:	HexCode.c
**
** Notes:       Used for encoding and decoding hex strings to/from byte arrays
**
*******************************************************************************/

/******************************************************************************/
/* Include Files*/
/******************************************************************************/
#include "HexCode.h"

/******************************************************************************/
/* Local Functions or Private Members*/
/******************************************************************************/

/*=============================================================================*/
/* Encode a byte array into hex, output buffer must be at least nLength in     */
/* length (or nLength+1 if null termination is required), nLength is the size  */
/* of the output buffer (excluding optional null terminating character)        */
/*=============================================================================*/
void
HexEncode(
    uint8_t *pInput,
    uint32_t nLength,
    uint8_t *pOutput,
    bool bUpperCase,
    bool bWithNullTermination
    )
{
    uint32_t nPos = 0;
    while (nPos < nLength)
    {
        pOutput[nPos] = (pInput[nPos/2] & 0xf0) / 16 + HEX_ENCODE_NUMERIC_ADDITION;
        pOutput[nPos+1] = (pInput[nPos/2] & 0x0f) + HEX_ENCODE_NUMERIC_ADDITION;
        if (pOutput[nPos] > '9')
        {
            pOutput[nPos] += (bUpperCase == true ? HEX_ENCODE_UPPER_CASE_ALPHA_ADDITION : HEX_ENCODE_LOWER_CASE_ALPHA_ADDITION);
        }
        if (pOutput[nPos+1] > '9')
        {
            pOutput[nPos+1] += (bUpperCase == true ? HEX_ENCODE_UPPER_CASE_ALPHA_ADDITION : HEX_ENCODE_LOWER_CASE_ALPHA_ADDITION);
        }
        nPos += 2;
    }

    if (bWithNullTermination == true)
    {
        pOutput[nPos] = 0;
    }
}

/*=============================================================================*/
/* Decode a hex string into a byte array, output buffer must be at least       */
/* nLength/2 in length, nLength is the size of the input buffer                */
/*=============================================================================*/
void
HexDecode(
    uint8_t *pInput,
    uint32_t nLength,
    uint8_t *pOutput
    )
{
    uint32_t nPos = 0;
    while (nPos < nLength)
    {
        pOutput[nPos/2] = (pInput[nPos] - (pInput[nPos] >= 'a' && pInput[nPos] <= 'f' ? HEX_DECODE_LOWER_CASE_ALPHA_SUBTRACT : (pInput[nPos] >= 'A' && pInput[nPos] <= 'F' ? HEX_DECODE_UPPER_CASE_ALPHA_SUBTRACT : HEX_DECODE_NUMERIC_SUBTRACT))) * 16;
        pOutput[nPos/2] += (pInput[nPos+1] - (pInput[nPos+1] >= 'a' && pInput[nPos+1] <= 'f' ? HEX_DECODE_LOWER_CASE_ALPHA_SUBTRACT : (pInput[nPos+1] >= 'A' && pInput[nPos+1] <= 'F' ? HEX_DECODE_UPPER_CASE_ALPHA_SUBTRACT : HEX_DECODE_NUMERIC_SUBTRACT)));
        nPos+=2;
    }
}

/******************************************************************************/
/* END OF FILE*/
/******************************************************************************/
