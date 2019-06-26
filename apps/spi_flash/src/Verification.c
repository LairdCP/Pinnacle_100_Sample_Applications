/******************************************************************************
**              Copyright (C) 2019 Laird Connectivity
**
** Project:     Pinnacle 100 Zephyr
**
** Module:	Verification.c
**
** Notes:       Contains checksum/hash verification functions
**
*******************************************************************************/

/******************************************************************************/
/* Include Files                                                              */
/******************************************************************************/
#include "verification.h"

/******************************************************************************/
/* Global/Static Variable Declarations                                        */
/******************************************************************************/
//Default padding element
uint8_t baPadding[] = {INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE};

/******************************************************************************/
/* Local Functions or Private Members                                         */
/******************************************************************************/

/*=============================================================================*/
/*=============================================================================*/
uint32_t GenerateCRC(uint8_t nType, void *data, uint32_t nSize, uint16_t nPadding, uint8_t nPaddingChar)
{
	uint32_t i = 0;
	uint32_t nSeg = 32;
	uint32_t nCRC = 0;
	while (i < nSize)
	{
		if ((i + nSeg) > nSize)
		{
			nSeg = nSize - i;
		}

		if (nType == CHECKSUM_NORDIC_CRC32)
		{
			nCRC = crc32_compute((const uint8_t*)(data + i), nSeg, &nCRC);
		}
		else if (nType == CHECKSUM_LAIRD_CRC32)
		{
			nCRC = MscPubCalc32bitCrcNonTableMethod(nCRC, (const uint8_t*)(data + i), nSeg);
		}

		i += nSeg;
	}

	if (nPadding > 0)
	{
		//Set padding character
		if (baPadding[0] != nPaddingChar)
		{
			memset(baPadding, nPaddingChar, sizeof(nPaddingChar));
		}

		//Add padding
		i = 0;
		nSeg = sizeof(baPadding);
		while (i < nPadding)
		{
			if ((i + nSeg) > nPadding)
			{
				nSeg = nPadding - i;
			}

			if (nType == CHECKSUM_NORDIC_CRC32)
			{
				nCRC = crc32_compute(baPadding, nSeg, &nCRC);
			}
			else if (nType == CHECKSUM_LAIRD_CRC32)
			{
				nCRC = MscPubCalc32bitCrcNonTableMethod(nCRC, baPadding, nSeg);
			}
			i += nSeg;
		}
	}

	return nCRC;
}

/******************************************************************************/
/* END OF FILE                                                                */
/******************************************************************************/
