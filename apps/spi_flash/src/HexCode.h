/******************************************************************************
**              Copyright (C) 2019 Laird Connectivity
**
** Project:     Pinnacle 100
**
** Module:	HexCode.h
**
** Notes:       Used for encoding and decoding hex strings to/from byte arrays
**
*******************************************************************************/
#ifndef HEXCODE_H__
#define HEXCODE_H__

/******************************************************************************/
/* Include Files*/
/******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/******************************************************************************/
/* Defines*/
/******************************************************************************/
#define HEX_DECODE_LOWER_CASE_ALPHA_SUBTRACT      0x57
#define HEX_DECODE_UPPER_CASE_ALPHA_SUBTRACT      0x37
#define HEX_DECODE_NUMERIC_SUBTRACT               0x30
#define HEX_ENCODE_LOWER_CASE_ALPHA_ADDITION      0x27
#define HEX_ENCODE_UPPER_CASE_ALPHA_ADDITION      0x7
#define HEX_ENCODE_NUMERIC_ADDITION               0x30

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
    );

/*=============================================================================*/
/* Decode a hex string into a byte array, output buffer must be at least       */
/* nLength/2 in length, nLength is the size of the input buffer                */
/*=============================================================================*/
void
HexDecode(
    uint8_t *pInput,
    uint32_t nLength,
    uint8_t *pOutput
    );

#endif
/******************************************************************************/
/* END OF FILE*/
/******************************************************************************/
