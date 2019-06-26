/******************************************************************************
**              Copyright (C) 2019 Laird Connectivity
**
** Project:     Pinnacle 100
**
** Module:	MscCRC32.c
**
** Notes:       Contains the public interface to the run time engine
**              Code taken from MscPublic.c from UwCore
**
*******************************************************************************/

/******************************************************************************/
/* Include Files*/
/******************************************************************************/
#include "MscCRC32.h"
#include <math.h>

/******************************************************************************/
/* Local Functions or Private Members*/
/******************************************************************************/

/*=============================================================================*/
#define CRC32_POLYNOMIAL 0xEDB88320
/*=============================================================================*/
static uint32_t
MscPubCalc32bitForByte(
    uint8_t nByte
    )
{
    uint8_t nBitCount=8;
    uint32_t nCrc32 = nByte;
    while(nBitCount--)
    {
        if(nCrc32 & 1)
        {
            nCrc32 = (nCrc32 >> 1)^CRC32_POLYNOMIAL;
        }
        else
        {
            nCrc32 >>= 1;
        }
    }

    return nCrc32;
}

/*=============================================================================*/
/*
** Given an array of bytes, a new 32 bit CRC is calculated using the slow
** method. Slow method because it is used once to calc lang hash.
*/
/*=============================================================================*/
uint32_t
MscPubCalc32bitCrcNonTableMethod(
    uint32_t nCrc32,
    const uint8_t *pSrcStr,
    uint32_t nSrcLen /* in bytes */
    )
{
    uint32_t nTemp1,nTemp2;
    while(nSrcLen--)
    {
        nTemp1 = (nCrc32 >> 8) & 0x00FFFFFFL;
        nTemp2 = (uint32_t)MscPubCalc32bitForByte( (uint8_t)((nCrc32 ^ *pSrcStr++) & 0xFF) );
        nCrc32 = nTemp1 ^ nTemp2;
    }

    return nCrc32;
}

/******************************************************************************/
/* END OF FILE*/
/******************************************************************************/
