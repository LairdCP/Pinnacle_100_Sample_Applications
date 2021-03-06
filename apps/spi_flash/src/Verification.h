/******************************************************************************
**              Copyright (C) 2019 Laird Connectivity
**
** Project:     Pinnacle 100 Zephyr
**
** Module:	Verification.h
**
** Notes:       Contains checksum/hash verification functions
**
*******************************************************************************/
#ifndef VERIFICATION_HEADER_H_
#define VERIFICATION_HEADER_H_

/******************************************************************************/
/* Include Files                                                              */
/******************************************************************************/
#include <stdint.h>
#include "crc32.h"
#include "MscCRC32.h"
#include "target.h"

/******************************************************************************/
/* Local Functions or Private Members                                         */
/******************************************************************************/

/*=============================================================================*/
/*=============================================================================*/
uint32_t GenerateCRC(uint8_t nType, void *data, uint32_t nSize, uint16_t nPadding, uint8_t nPaddingChar);

#endif
/******************************************************************************/
/* END OF FILE                                                                */
/******************************************************************************/
