
#include <zephyr.h>
#include <flash.h>
#include <device.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tinycrypt/sha256.h"
#include "target.h"
#include "FOTA.h"
#include "MscCRC32.h"
#include "verification.h"
#include "BootloaderSettings.h"
#include "HexCode.h"

#define LOG_DOMAIN laird_fota
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);


#define LAIRD_FOTA_DEBUG_LOG  //Outputs debug information 
//#define FOTA_WRITE_DEBUGGING  //Will generate a LOT of debug when doing partition writes
//#define FOTA_USE_MALLOC

#ifndef FOTA_USE_MALLOC
UpgradeDataStruct UpgradeInfoObject;
#endif
UpgradeDataStruct *UpgradeInfo = NULL;
struct device *flash_dev;

static uint8_t IsPartitionOpen = false; //True if a Partition is open for writing
static int8_t CurrentPartition = -1; //The current open Partition
static uint32_t PartitionSize = 0; //Size of the current buffer for a Partition (excluding temporary write buffer)
static uint32_t PartitionErasedSize = 0; //Size of erased Partitions ready for writing new data

static uint8_t nWriteBuffer[QSPI_MIN_WRITE_SIZE];
static uint8_t nWriteBufferPos = 0;
static uint32_t nLastEraseSector = 0;
static uint32_t PartitionStartOffset = 0;

//Functions
int32_t LairdFWUpgrade_Init()
{
	//Initialises the driver
	flash_dev = device_get_binding(QSPI_FLASH_LABEL);

	if (!flash_dev)
	{
		//Driver was not enabled
		LOG_ERR("SPI flash driver was not found.");
		return LAIRD_ERROR_CODE_FLASH_INIT_FAILED;
	}

#ifdef FOTA_USE_MALLOC
	UpgradeInfo = malloc(sizeof(UpgradeDataStruct));
#else
	UpgradeInfo = &UpgradeInfoObject;
#endif
	memset(UpgradeInfo, 0, sizeof(UpgradeDataStruct));

	return LAIRD_ERROR_CODE_SUCCESS;
}

int8_t LairdFWUpgrade_Destroy()
{
	//Cleans up object creation
	if (UpgradeInfo != NULL)
	{
		memset(UpgradeInfo, 0, sizeof(UpgradeDataStruct));
#ifdef FOTA_USE_MALLOC
		free(UpgradeInfo);
#endif
		UpgradeInfo = NULL;
	}

	return LAIRD_ERROR_CODE_SUCCESS;
}

int32_t LairdFWUpgrade_LoadData()
{
	//Loads data from the memory chip
	void *pCPos = (void*)UpgradeInfo;
	uint32_t unReadAmt = TRANSFER_SIZE_BYTES;
	uint32_t unOffset = 0;
	uint32_t unReadLeft = sizeof(UpgradeDataStruct);
	bool bChecksumValid = false;

	while (unReadLeft > 0)
	{
		if (unReadAmt > unReadLeft)
		{
			unReadAmt = unReadLeft;
		}

		if (flash_read(flash_dev, unOffset, pCPos, unReadAmt) != 0)
		{
			//Read failed
			LOG_ERR("Failed to read from flash, offset 0x%lx", unOffset);
		}

		unOffset += unReadAmt;
		unReadLeft -= unReadAmt;
		pCPos += unReadAmt;
	}

	//Check if magic string is present
	if (memcmp(UpgradeInfo->MagicHeader, MAGIC_HEADER_VALUE, sizeof(UpgradeInfo->MagicHeader)) == 0)
	{
		//Magic header valid
		if (GenerateCRC(CHECKSUM_LAIRD_CRC32, &UpgradeInfo->UpdateVersion, sizeof(UpgradeDataStruct) - ((uint8_t*)&UpgradeInfo->UpdateVersion - (uint8_t*)&UpgradeInfo->MagicHeader), 0, 0) == UpgradeInfo->Checksum)
		{
			//Checksum is valid
			bChecksumValid = true;
			LOG_DBG("Main header checksum valid");
		}
	}

	if (bChecksumValid == false)
	{
		//Checksum or magic string is not valid, check the backup sector
		LOG_DBG("Main header checksum not valid");
		unReadAmt = TRANSFER_SIZE_BYTES;
		unOffset = QSPI_FLASH_BACKUP_HEADER_ADDRESS;
		unReadLeft = sizeof(UpgradeDataStruct);
		while (unReadLeft > 0)
		{
			if (unReadAmt > unReadLeft)
			{
				unReadAmt = unReadLeft;
			}

			if (flash_read(flash_dev, unOffset, pCPos, unReadAmt) != 0)
			{
				//Read failed
				LOG_ERR("SPI read failed at offset 0x%lx", unOffset);
			}

			unOffset += unReadAmt;
			unReadLeft -= unReadAmt;
			pCPos += unReadAmt;
		}

		//Check if magic string is present
		if (memcmp(UpgradeInfo->MagicHeader, MAGIC_HEADER_VALUE, sizeof(UpgradeInfo->MagicHeader)) == 0)
		{
			//Magic header valid
			if (GenerateCRC(CHECKSUM_LAIRD_CRC32, &UpgradeInfo->UpdateVersion, sizeof(UpgradeDataStruct) - ((uint8_t*)&UpgradeInfo->UpdateVersion - (uint8_t*)&UpgradeInfo->MagicHeader), 0, 0) == UpgradeInfo->Checksum)
			{
				//Checksum is valid
				bChecksumValid = true;
				LOG_DBG("Backup header checksum valid");
			}
		}
	}

	if (bChecksumValid == true)
	{
		LOG_DBG("Loaded QSPI partition data successfully");
		return LAIRD_ERROR_CODE_SUCCESS;
	}

	LOG_DBG("Failed to load QSPI partition data");
	return LAIRD_ERROR_CODE_INVALID_DATA;
}

int32_t LairdFWUpgrade_SaveData(bool bSaveBackup)
{
	//Saves the data to the memory chip
	void *pCPos = (void*)UpgradeInfo;
	uint32_t unReadAmt;
	uint32_t unOffset = QSPI_FLASH_MAIN_HEADER_ADDRESS;
	uint32_t unReadLeft = sizeof(UpgradeDataStruct);

	//Generate checksum
	UpgradeInfo->Checksum = GenerateCRC(CHECKSUM_LAIRD_CRC32, &UpgradeInfo->UpdateVersion, sizeof(UpgradeDataStruct) - ((uint8_t*)&UpgradeInfo->UpdateVersion - (uint8_t*)&UpgradeInfo->MagicHeader), 0, 0);

	//Erase memory
	while (unOffset < unReadLeft)
	{
		flash_write_protection_set(flash_dev, false);
		uint32_t dz = flash_erase(flash_dev, unOffset, FLASH_SECTOR_SIZE);
		LOG_DBG("QSPI Sector erase %lx %ld\n", unOffset, dz);
		flash_write_protection_set(flash_dev, true);
		unOffset += FLASH_SECTOR_SIZE;
	}

	unOffset = 0;
	unReadLeft = sizeof(UpgradeDataStruct);
	unReadAmt = TRANSFER_SIZE_BYTES;
	LOG_DBG("Start offset: %lx", unOffset);
	while (unReadLeft > 0)
	{
		if (unReadAmt > unReadLeft)
		{
			unReadAmt = unReadLeft;
		}

		flash_write_protection_set(flash_dev, false);
		if (flash_write(flash_dev, unOffset, pCPos, unReadAmt) != 0)
		{
			//Read failed
			LOG_ERR("SPI read failed at unOffset 0x%lx\n", unOffset);
		}
		flash_write_protection_set(flash_dev, true);

		unOffset += unReadAmt;
		unReadLeft -= unReadAmt;
		pCPos += unReadAmt;
	}
	LOG_DBG("End offset: %lx", unOffset);
	flash_write_protection_set(flash_dev, true);

	void *pCPosA = (void*)UpgradeInfo;
	uint32_t unReadAmtA = TRANSFER_SIZE_BYTES;
	uint32_t unOffsetA = 0;
	uint32_t unReadLeftA = sizeof(UpgradeDataStruct);

	while (unReadLeftA > 0)
	{
		if (unReadAmtA > unReadLeftA)
		{
			unReadAmtA = unReadLeftA;
		}

		if (flash_read(flash_dev, unOffsetA, pCPosA, unReadAmtA) != 0)
		{
			//Read failed
			LOG_ERR("SPI read failed at offset 0x%lx", unOffset);
		}

		unOffsetA += unReadAmtA;
		unReadLeftA -= unReadAmtA;
		pCPosA += unReadAmtA;
	}

	//next do backup section
	if (bSaveBackup == true)
	{
		//Erase memory
		unOffset = QSPI_FLASH_BACKUP_HEADER_ADDRESS;
		unReadLeft = QSPI_FLASH_BACKUP_HEADER_ADDRESS + sizeof(UpgradeDataStruct);
		while (unOffset < unReadLeft)
		{
			flash_write_protection_set(flash_dev, false);
			flash_erase(flash_dev, unOffset, FLASH_SECTOR_SIZE);
			LOG_DBG("QSPI sector erase %lx", unOffset);
			flash_write_protection_set(flash_dev, true);
			unOffset += FLASH_SECTOR_SIZE;
		}

		pCPos = (void*)UpgradeInfo;
		unOffset = QSPI_FLASH_BACKUP_HEADER_ADDRESS;
		unReadLeft = sizeof(UpgradeDataStruct);
		unReadAmt = TRANSFER_SIZE_BYTES;
		LOG_DBG("Start offset: %lx", unOffset);
		while (unReadLeft > 0)
		{
			if (unReadAmt > unReadLeft)
			{
				unReadAmt = unReadLeft;
			}

			flash_write_protection_set(flash_dev, false);
			if (flash_write(flash_dev, unOffset, pCPos, unReadAmt) != 0)
			{
				//Read failed
				LOG_ERR("SPI read failed at unOffset 0x%lx", unOffset);
			}
			flash_write_protection_set(flash_dev, true);

			unOffset += unReadAmt;
			unReadLeft -= unReadAmt;
			pCPos += unReadAmt;
		}
		LOG_DBG("End offset: %lx", unOffset);
		flash_write_protection_set(flash_dev, true);
	}

	//
	return LAIRD_ERROR_CODE_SUCCESS;
}

void LairdFWUpgrade_CreateEmpty()
{
	//Creates an empty QSPI partition array
	memset(UpgradeInfo, INTERNAL_FLASH_PAD_BYTE, sizeof(UpgradeDataStruct));

	//Add the magic header
	memcpy(UpgradeInfo->MagicHeader, MAGIC_HEADER_VALUE, sizeof(MAGIC_HEADER_VALUE));

	//Set required values
	UpgradeInfo->BootFails = 0;
	UpgradeInfo->ForceUpdate = false;
	UpgradeInfo->SectionsPresent = 0;
	UpgradeInfo->UpdateVersion = 1;
}

SectionStruct *LairdFWUpgrade_GetPartitionData(uint8_t nPartition)
{
	if (nPartition >= SECTIONS_PRESENT)
	{
		//Invalid partition
		return NULL;
	}
	return &UpgradeInfo->SectionInfo[nPartition];
}

int8_t LairdFWUpgrade_NextFreePartition()
{
	//Returns the ID of the next available Partition or -1 if they are all used
	if (UpgradeInfo->SectionsPresent < SECTIONS_PRESENT)
	{
		return UpgradeInfo->SectionsPresent;
	}

	//All sections used
	return LAIRD_ERROR_CODE_ALL_SECTIONS_USED;
}

int8_t LairdFWUpgrade_ClearAll()
{
	//Clear the flash memory
#ifdef TODO
	int8_t ReturnCode = 0;
	flash_write_protection_set(flash_dev, false);
	if (flash_erase(flash_dev, 0, QSPI_FLASH_SIZE) != 0)
	{
		ReturnCode = 1;
		LOG_ERR("Flash erase failed!\n");
	}
	flash_write_protection_set(flash_dev, true);

	if (ReturnCode == 0)
	{
		//Clear all array values
		memset(UpgradeInfo, 0, sizeof(UpgradeDataStruct));
	}
	return ReturnCode;
#endif

	return LAIRD_ERROR_CODE_NOT_IMPLEMENTED;
}

int8_t LairdFWUpgrade_OpenPartition(uint8_t unPartition)
{
	//Open specified Partition for writing
	if (IsPartitionOpen == true)
	{
		//Partition is already open
		return LAIRD_ERROR_CODE_PARTITION_ALREADY_OPEN;
	}

	//Open Partition
	CurrentPartition = unPartition;
	PartitionSize = 0;
	PartitionErasedSize = 0;
	IsPartitionOpen = true;

	//Deal with partition array
	if (unPartition == 0)
	{
		//Start at correct sector for first partition
		UpgradeInfo->SectionInfo[CurrentPartition].SectionStart = QSPI_FLASH_FIRST_PARTITION_ADDRESS;
	}
	else
	{
		//Start at next offset from previous image
		int8_t nCheckPartition = CurrentPartition-1;
		UpgradeInfo->SectionInfo[CurrentPartition].SectionStart = 0;
		while (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart == 0 && nCheckPartition >= 0)
		{
			if (UpgradeInfo->SectionInfo[nCheckPartition].SectionEnd != 0)
			{
				UpgradeInfo->SectionInfo[CurrentPartition].SectionStart = UpgradeInfo->SectionInfo[nCheckPartition].SectionEnd;
				if ((UpgradeInfo->SectionInfo[CurrentPartition].SectionStart % FLASH_SECTOR_SIZE) != FLASH_SECTOR_SIZE)
				{
					//Round up
					UpgradeInfo->SectionInfo[CurrentPartition].SectionStart += (FLASH_SECTOR_SIZE-UpgradeInfo->SectionInfo[CurrentPartition].SectionStart %FLASH_SECTOR_SIZE);
				}
			}
			--nCheckPartition;
		}

		if (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart == 0)
		{
			//Start at correct sector for first partition
			UpgradeInfo->SectionInfo[CurrentPartition].SectionStart = QSPI_FLASH_FIRST_PARTITION_ADDRESS;
		}
	}
	PartitionStartOffset = UpgradeInfo->SectionInfo[CurrentPartition].SectionStart;

	LOG_DBG("Opened %d", CurrentPartition);
	LOG_DBG("Start address: %lx", UpgradeInfo->SectionInfo[CurrentPartition].SectionStart);

	UpgradeInfo->SectionInfo[CurrentPartition].SectionEnd = UpgradeInfo->SectionInfo[CurrentPartition].SectionStart;
	UpgradeInfo->SectionInfo[CurrentPartition].SectionSize = 0;

	nWriteBufferPos = 0;
	nLastEraseSector = 0;

	return LAIRD_ERROR_CODE_SUCCESS;
}

int8_t LairdFWUpgrade_IsPartitionOpen()
{
	//Returns true if a Partition is open
	return IsPartitionOpen;
}

int8_t LairdFWUpgrade_PartitionOpenNumber()
{
	//Returns the current open Partition number, or an error if a partition is not open
	if (IsPartitionOpen == false)
	{
		return LAIRD_ERROR_CODE_PARTITION_NOT_OPEN;
	}

	return CurrentPartition;
}

int8_t LairdFWUpgrade_ClosePartition(int8_t unOptions)
{
	//Closes the currently open Partition and updates the header if specified
	if (IsPartitionOpen != true)
	{
		//Partition is not open
		return LAIRD_ERROR_CODE_PARTITION_NOT_OPEN;
	}

	uint32_t nStartPos;
	uint32_t nEndPos;
	uint8_t nChecksumType = UpgradeInfo->SectionInfo[CurrentPartition].ChecksumType;
	uint32_t nExpectedCRC;

	if (PartitionSize != 0)
	{
		//Set the correct sizes
		UpgradeInfo->SectionInfo[CurrentPartition].SectionStart = PartitionStartOffset;
		UpgradeInfo->SectionInfo[CurrentPartition].SectionSize = PartitionSize;
		UpgradeInfo->SectionInfo[CurrentPartition].SectionEnd = PartitionStartOffset + PartitionSize;
	}

	if (nWriteBufferPos != 0)
	{
		//Append final part of data - pad with flash bytes
		memset(&nWriteBuffer[nWriteBufferPos], INTERNAL_FLASH_PAD_BYTE, sizeof(nWriteBuffer)-nWriteBufferPos);

		//Write data out
		flash_write_protection_set(flash_dev, false);
		flash_write(flash_dev, (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart + PartitionSize), nWriteBuffer, sizeof(nWriteBuffer));
		flash_write_protection_set(flash_dev, true);
		PartitionSize += nWriteBufferPos;
		nWriteBufferPos = 0;
	}

	//Deal with partition array details
	if ((unOptions & PARTITION_OPTIONS_NULL_SIZE) == PARTITION_OPTIONS_NULL_SIZE)
	{
		//Null size
		UpgradeInfo->SectionInfo[CurrentPartition].SectionSize = 0;
		UpgradeInfo->SectionInfo[CurrentPartition].SectionStart = 0;
		UpgradeInfo->SectionInfo[CurrentPartition].SectionEnd = 0;
		UpgradeInfo->SectionInfo[CurrentPartition].TargetSize = 0;
		UpgradeInfo->SectionInfo[CurrentPartition].TargetStart = 0;
		UpgradeInfo->SectionInfo[CurrentPartition].TargetEnd = 0;
	}
	else
	{
		//Include size
		UpgradeInfo->SectionInfo[CurrentPartition].SectionSize = PartitionSize;
		UpgradeInfo->SectionInfo[CurrentPartition].SectionEnd = UpgradeInfo->SectionInfo[CurrentPartition].SectionStart + PartitionSize;

		if (UpgradeInfo->SectionInfo[CurrentPartition].Compressed == 0)
		{
			//Section is not compressed, update target sizes
			UpgradeInfo->SectionInfo[CurrentPartition].TargetSize = UpgradeInfo->SectionInfo[CurrentPartition].SectionSize;
			UpgradeInfo->SectionInfo[CurrentPartition].TargetEnd = UpgradeInfo->SectionInfo[CurrentPartition].TargetStart + UpgradeInfo->SectionInfo[CurrentPartition].TargetSize;
		}
	}

	nStartPos = UpgradeInfo->SectionInfo[CurrentPartition].SectionStart;
	nEndPos = UpgradeInfo->SectionInfo[CurrentPartition].SectionEnd;

	if ((CurrentPartition + 1) > UpgradeInfo->SectionsPresent)
	{
		//Increment number of partitions
		UpgradeInfo->SectionsPresent = (CurrentPartition + 1);
	}


#ifdef LAIRD_FOTA_DEBUG_LOG
	LOG_DBG("ON_CLOSE: Current Partition %d, PartitionSize %ld, PartitionErasedSize %ld", CurrentPartition, PartitionSize, PartitionErasedSize);
	LOG_DBG("Section %d:\n", CurrentPartition);
	LOG_DBG("\n\tSection Start: 0x%lx\n\tSection End: 0x%lx\n\tSection Size: 0x%lx\n\tTarget: %d\n\tTarget Start: 0x%lx\n\tTarget End: 0x%lx", UpgradeInfo->SectionInfo[CurrentPartition].SectionStart, UpgradeInfo->SectionInfo[CurrentPartition].SectionEnd, UpgradeInfo->SectionInfo[CurrentPartition].SectionSize, UpgradeInfo->SectionInfo[CurrentPartition].Target, UpgradeInfo->SectionInfo[CurrentPartition].TargetStart, UpgradeInfo->SectionInfo[CurrentPartition].TargetEnd);
	LOG_DBG("\n\tTarget Size: 0x%lx\n\tCompressed: %d\n\tVersion: %d\n\tForce %d\n\tChecksum Type: %d\n\tImage Checksum: 0x%lx", UpgradeInfo->SectionInfo[CurrentPartition].TargetSize, UpgradeInfo->SectionInfo[CurrentPartition].Compressed, UpgradeInfo->SectionInfo[CurrentPartition].Version, UpgradeInfo->SectionInfo[CurrentPartition].Force, UpgradeInfo->SectionInfo[CurrentPartition].ChecksumType, UpgradeInfo->SectionInfo[CurrentPartition].ImageChecksum);
	LOG_DBG("\n\tTarget Checksum: 0x%lx\n\tSignature Type: %d", UpgradeInfo->SectionInfo[CurrentPartition].TargetChecksum, UpgradeInfo->SectionInfo[CurrentPartition].SignatureType);
	uint8_t baTmpBuf[SIGNATURE_SIZE * 2 + 1];
	HexEncode(UpgradeInfo->SectionInfo[CurrentPartition].Signature, SIGNATURE_SIZE * 2, baTmpBuf, true, true);
	LOG_DBG("\n\tSignature (hex): %s", log_strdup(baTmpBuf));
#ifdef PARTITION_DATA_FILENAME_REVERSE
	uint8_t a = 0;
	uint8_t b = 0;
	while (b < MAX_FILENAME_SIZE)
	{
		if (UpgradeInfo->SectionInfo[CurrentPartition].Filename[b] == NULL)
		{
			break;
		}
		++b;
	}
	uint8_t c = 0;
	while (a < b)
	{
		c = ((UpgradeInfo->SectionInfo[CurrentPartition].Filename[a] & 0xf) << 4) | ((UpgradeInfo->SectionInfo[CurrentPartition].Filename[a] & 0xf0) >> 4);
		baTmpBuf[a] = c;
		++a;
	}
	baTmpBuf[a] = 0;
#else
	l = 0;
	while (l < MAX_FILENAME_SIZE)
	{
		baTmpBuf[a] = UpgradeInfo->SectionInfo[CurrentPartition].Filename[l];
		++l;
	}
	baTmpBuf[l] = 0;
#endif
	LOG_DBG("\n\tFilename: %s", log_strdup(baTmpBuf));

	bool bPrintData = true;
	a = 0;
	b = MAX_EXTRA_DATA_SIZE;
	while (a < b)
	{
		if (UpgradeInfo->SectionInfo[CurrentPartition].ExtraData[a] > '~' || (UpgradeInfo->SectionInfo[CurrentPartition].ExtraData[a] > NULL && UpgradeInfo->SectionInfo[CurrentPartition].ExtraData[a] < ' ' && UpgradeInfo->SectionInfo[CurrentPartition].ExtraData[a] != '\r' && UpgradeInfo->SectionInfo[CurrentPartition].ExtraData[a] != '\n'))
		{
			bPrintData = false;
		}
		else if (UpgradeInfo->SectionInfo[CurrentPartition].ExtraData[a] == NULL)
		{
			break;
		}
		++a;
	}

	if (bPrintData == true)
	{
		uint8_t nTmpChar;
		if (a == MAX_EXTRA_DATA_SIZE)
		{
			//No null character detected, insert one temporarily
			nTmpChar = UpgradeInfo->SectionInfo[CurrentPartition].ExtraData[MAX_EXTRA_DATA_SIZE-1];
			UpgradeInfo->SectionInfo[CurrentPartition].ExtraData[MAX_EXTRA_DATA_SIZE-1] = NULL;
		}
		LOG_DBG("\n\tExtra data (string): %s", log_strdup(baTmpBuf));
		if (a == MAX_EXTRA_DATA_SIZE)
		{
			//Return character
			UpgradeInfo->SectionInfo[CurrentPartition].ExtraData[MAX_EXTRA_DATA_SIZE-1] = NULL;
		}
	}

	HexEncode(UpgradeInfo->SectionInfo[CurrentPartition].ExtraData, MAX_EXTRA_DATA_SIZE*2, baTmpBuf, true, true);
	LOG_DBG("\n\tExtra data (hex): %s\n\tEncryption key: %d", log_strdup(baTmpBuf), UpgradeInfo->SectionInfo[CurrentPartition].EncryptionKey);
#endif

	nExpectedCRC = UpgradeInfo->SectionInfo[CurrentPartition].ImageChecksum;

	//Close Partition
	IsPartitionOpen = false;
	CurrentPartition = -1;
	PartitionSize = 0;
	PartitionErasedSize = 0;

	if ((unOptions & PARTITION_OPTIONS_CHECK_CHECKSUM) == PARTITION_OPTIONS_CHECK_CHECKSUM && nChecksumType != CHECKSUM_BYPASS)
	{
		uint8_t nSegSize = TRANSFER_SIZE_BYTES;
		uint8_t TmpBuf[TRANSFER_SIZE_BYTES];
		uint32_t nCRC = 0;
		while (nStartPos < nEndPos)
		{
			if (nSegSize > nEndPos - nStartPos)
			{
				nSegSize = nEndPos - nStartPos;
			}
			flash_read(flash_dev, nStartPos, TmpBuf, nSegSize);

			if (nChecksumType == CHECKSUM_NORDIC_CRC16)
			{
//TODO
			}
			else if (nChecksumType == CHECKSUM_NORDIC_CRC32)
			{
//				nCRC = crc32_compute(TmpBuf, nSegSize, &nCRC);
			}
			else if (nChecksumType == CHECKSUM_LAIRD_CRC32)
			{
				nCRC = MscPubCalc32bitCrcNonTableMethod(nCRC, TmpBuf, nSegSize);
			}
			nStartPos += nSegSize;
		}

		LOG_DBG("Checksum: %lx", nCRC);

		if (nCRC != nExpectedCRC)
		{
			//Checksum failure
			return LAIRD_ERROR_CODE_CHECKSUM_FAIL;
		}
	}

	return LAIRD_ERROR_CODE_SUCCESS;
}

int32_t LairdFWUpgrade_WritePartition(uint8_t *Data, uint32_t DataSize)
{
	//Appends data to currently active Partition, automatically clearing the data before writing if required
	if (IsPartitionOpen != true)
	{
		//Partition is not open
		return LAIRD_ERROR_CODE_PARTITION_NOT_OPEN;
	}

	if (nWriteBufferPos != 0)
	{
#ifdef FOTA_WRITE_DEBUGGING
		LOG_DBG("nWriteBufferPos = %d", nWriteBufferPos);
#endif
		//We have an offset to write
		uint8_t nWriteSegmentSize = sizeof(nWriteBuffer) - nWriteBufferPos;
		if (nWriteSegmentSize > DataSize)
		{
			nWriteSegmentSize = DataSize;
		}
#ifdef FOTA_WRITE_DEBUGGING
		LOG_DBG("Append %d", nWriteSegmentSize);
#endif
		memcpy(&nWriteBuffer[nWriteBufferPos], Data, nWriteSegmentSize);
		nWriteBufferPos += nWriteSegmentSize;

		if (nWriteBufferPos == QSPI_MIN_WRITE_SIZE)
		{
			//Write this data out
			flash_write_protection_set(flash_dev, false);
			flash_write(flash_dev, (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart + PartitionSize), nWriteBuffer, sizeof(nWriteBuffer));
			flash_write_protection_set(flash_dev, true);
			nWriteBufferPos = 0;
			PartitionSize += sizeof(nWriteBuffer);
			PartitionErasedSize -= sizeof(nWriteBuffer);
			DataSize -= nWriteSegmentSize;
			Data += nWriteSegmentSize;
#ifdef FOTA_WRITE_DEBUGGING
			LOG_DBG("Write out %d, data %d, size %d -> New offset %x", sizeof(nWriteBuffer), Data, DataSize, (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart + PartitionSize));
#endif
		}
		else
		{
			//Not enough data so prevent further processing
			return LAIRD_ERROR_CODE_SUCCESS;
		}
	}

	if (DataSize == 0)
	{
		return;
	}

	//Erase extra data size if needed
	if (PartitionErasedSize < (DataSize + (QSPI_MIN_WRITE_SIZE - DataSize%QSPI_MIN_WRITE_SIZE)))
	{
		//Erase a Partition
		flash_write_protection_set(flash_dev, false);
		if (nLastEraseSector == 0)
		{
			nLastEraseSector = UpgradeInfo->SectionInfo[CurrentPartition].SectionStart;
		}
		flash_erase(flash_dev, nLastEraseSector, FLASH_SECTOR_SIZE);
		flash_write_protection_set(flash_dev, true);
		LOG_DBG("ERASE: %lx %x", nLastEraseSector, FLASH_SECTOR_SIZE);
		PartitionErasedSize += FLASH_SECTOR_SIZE;
		nLastEraseSector += FLASH_SECTOR_SIZE;
	}

	//Write out as much data as possible
	flash_write_protection_set(flash_dev, false);
	if ((DataSize%QSPI_MIN_WRITE_SIZE) != 0)
	{
#ifdef FOTA_WRITE_DEBUGGING
		LOG_DBG("unequal");
#endif
		//Unequal number of bytes
		uint32_t nBytesWritten = 0;
		if (DataSize >= QSPI_MIN_WRITE_SIZE)
		{
			nBytesWritten = DataSize - (DataSize%QSPI_MIN_WRITE_SIZE);
			flash_write(flash_dev, (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart + PartitionSize), Data, nBytesWritten);
			LOG_DBG("written %ld at %lx", nBytesWritten, (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart + PartitionSize));
			PartitionSize += nBytesWritten;
			PartitionErasedSize -= nBytesWritten;
		}

		memcpy(nWriteBuffer, &Data[nBytesWritten], DataSize - nBytesWritten);
		nWriteBufferPos += DataSize - nBytesWritten;
#ifdef FOTA_WRITE_DEBUGGING
		LOG_DBG("copied %d bytes, now %d", DataSize - nBytesWritten, nWriteBufferPos);
#endif

	}
	else
	{
		//Equal number of bytes
#ifdef FOTA_WRITE_DEBUGGING
		LOG_DBG("equal");
#endif
		uint32_t nOldStart = (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart + PartitionSize) / QSPI_PAGE_SIZE * QSPI_PAGE_SIZE;
		uint32_t nNewStart = (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart + PartitionSize + DataSize) / QSPI_PAGE_SIZE * QSPI_PAGE_SIZE;
		if (nOldStart != nNewStart)
		{
			//Crosses page boundary
			uint32_t nPageLeft = (QSPI_PAGE_SIZE - (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart + PartitionSize)%256);
			LOG_DBG("Old start: %ld, new start: %ld, Left: %ld", nOldStart, nNewStart, nPageLeft);

			//Write remaining page
			flash_write(flash_dev, (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart + PartitionSize), Data, nPageLeft);
			flash_write_protection_set(flash_dev, true);

			//Write next page
			flash_write_protection_set(flash_dev, false);
			flash_write(flash_dev, (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart + PartitionSize + nPageLeft), &Data[nPageLeft], DataSize-nPageLeft);
		}
		else
		{
			//Does not cross page boundary
			flash_write(flash_dev, (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart + PartitionSize), Data, DataSize);
			LOG_DBG("Written %ld @ %lx", DataSize, (UpgradeInfo->SectionInfo[CurrentPartition].SectionStart + PartitionSize));
		}

		PartitionSize += DataSize;
		PartitionErasedSize -= DataSize;
	}
	flash_write_protection_set(flash_dev, true);
	LOG_DBG("Erased size %lx, Partition size %lx", PartitionErasedSize, PartitionSize);

	return LAIRD_ERROR_CODE_SUCCESS;
}


int8_t LairdFWUpgrade_UsedPartitions()
{
	//Returns the number of used Partitions
	return UpgradeInfo->SectionsPresent;
}

int8_t LairdFWUpgrade_FreePartitions()
{
	//Returns the number of free Partitions
	return SECTIONS_PRESENT-UpgradeInfo->SectionsPresent;
}

int32_t LairdFWUpgrade_UsedPartitionSpace(uint8_t Partition)
{
	//Returns the amount of space (in bytes) which is used by a Partition
	return UpgradeInfo->SectionInfo[Partition].SectionSize;
}

int32_t LairdFWUpgrade_TotalUsedSpaceNoGaps()
{
	//Returns the total amount of space used (in bytes) on the memory chip by Partitions and the header (not including gaps in the data)
	uint8_t i = 0;
	int32_t SpaceUsed = 0;
	while (i < UpgradeInfo->SectionsPresent)
	{
		SpaceUsed += UpgradeInfo->SectionInfo[i].SectionSize;
		++i;
	}
	return SpaceUsed + sizeof(UpgradeDataStruct) * 2;
}

int32_t LairdFWUpgrade_TotalUsedSpace()
{
	//Returns the total amount of space used (in bytes) on the memory chip by Partitions and the header (including gaps in the data)
	if (UpgradeInfo->SectionsPresent == 0)
	{
		//No data present
		return QSPI_FLASH_FIRST_PARTITION_ADDRESS;
	}

	int32_t SpaceUsed = UpgradeInfo->SectionInfo[(UpgradeInfo->SectionsPresent-1)].SectionEnd;
	if ((SpaceUsed % FLASH_SECTOR_SIZE) != FLASH_SECTOR_SIZE)
	{
		//Round up
		SpaceUsed += (FLASH_SECTOR_SIZE-SpaceUsed%FLASH_SECTOR_SIZE);
	}
	return SpaceUsed;
}

int32_t LairdFWUpgrade_TotalFreeSpace()
{
	//Returns the total free space available (in bytes) on the memory chip
	return LairdFWUpgrade_TotalSpace()-LairdFWUpgrade_TotalUsedSpace();
}

int32_t LairdFWUpgrade_TotalSpace()
{
	//Returns the total space (in bytes) on the memory chip
	return QSPI_FLASH_SIZE;
}

int8_t LairdFWUpgrade_DeletePartition(uint8_t unPartition)
{
	//Removes a partition section from the QSPI flash and will shift down all data in partitions above it
	if (UpgradeInfo->SectionsPresent == 0)
	{
		//No Partitions to delete
		return LAIRD_ERROR_CODE_NO_PARTITIONS;
	}
	else if (unPartition > UpgradeInfo->SectionsPresent)
	{
		//Partition is not in use so cannot be deleted
		return LAIRD_ERROR_CODE_PARTITION_IN_USE;
	}

	uint8_t i = unPartition;

	while (i < UpgradeInfo->SectionsPresent)
	{
		memmove(&UpgradeInfo->SectionInfo[i], &UpgradeInfo->SectionInfo[i+1], sizeof(SectionStruct));
		++i;
	}

	--UpgradeInfo->SectionsPresent;
	memset(&UpgradeInfo->SectionInfo[UpgradeInfo->SectionsPresent], INTERNAL_FLASH_PAD_BYTE, sizeof(SectionStruct));

	//Save the memory header section
	LairdFWUpgrade_SaveData(true);

	//Complete
	return LAIRD_ERROR_CODE_SUCCESS;
}

void LairdFWUpgrade_Defragment()
{
	uint32_t nNextPartitionExpectedStart = QSPI_FLASH_FIRST_PARTITION_ADDRESS;
	uint32_t nReadPos;
	uint32_t nWritePos;
	uint32_t nCurPos;
	uint32_t nErasedSize;
	uint8_t i = 0;
	__ALIGN(4) uint8_t baTmpBuf[TRANSFER_SIZE_BYTES];

	while (i < UpgradeInfo->SectionsPresent)
	{
		if (UpgradeInfo->SectionInfo[i].SectionStart != nNextPartitionExpectedStart && UpgradeInfo->SectionInfo[i].SectionSize > 0)
		{
			nReadPos = UpgradeInfo->SectionInfo[i].SectionStart;
			nWritePos = nNextPartitionExpectedStart;
			nCurPos = 0;
			nErasedSize = 0;

			while (nCurPos < UpgradeInfo->SectionInfo[i].SectionSize)
			{
				if (nErasedSize == 0)
				{
					//Erase next block
					flash_write_protection_set(flash_dev, false);
					flash_erase(flash_dev, nWritePos, FLASH_SECTOR_SIZE);
					flash_write_protection_set(flash_dev, true);

					nErasedSize += QSPI_SECTOR_SIZE;
				}

				//
				uint32_t CopyAmount = TRANSFER_SIZE_BYTES;
				if (CopyAmount > nErasedSize)
				{
					CopyAmount = nErasedSize;
				}
				if (CopyAmount > (UpgradeInfo->SectionInfo[i].SectionSize - nCurPos))
				{
					CopyAmount = (UpgradeInfo->SectionInfo[i].SectionSize - nCurPos);
				}

				//Check if data is aligned to a 4-byte boundary
				uint32_t nWriteSize = CopyAmount;
				if ((nWriteSize & 0b11) != 0)
				{
					//Align to 4-byte boundary for reading/writing
					nWriteSize &= (0xffffffff - 0b11);
					nWriteSize += 0b100;
				}

				//
				if (flash_read(flash_dev, (int)nReadPos, baTmpBuf, nWriteSize) != 0)
				{
					//Read failed
while (1);
//TODO
//					BootloaderError(true, "QSPI flash read", 0);
				}

				if (nWriteSize != CopyAmount)
				{
					//
					memset(&baTmpBuf[CopyAmount], QSPI_FLASH_PAD_BYTE, nWriteSize - CopyAmount);
				}

				flash_write_protection_set(flash_dev, false);
				if (flash_write(flash_dev, (int)nWritePos, baTmpBuf, nWriteSize) != 0)
				{
					//Write failed
while (1);
//TODO
//					BootloaderError(true, "QSPI flash write", 0);
				}
				flash_write_protection_set(flash_dev, true);

				nReadPos += CopyAmount;
				nWritePos += CopyAmount;
				nCurPos += CopyAmount;
				nErasedSize -= CopyAmount;
			}

			//Erase the section where the previous data was
			if ((nWritePos % QSPI_SECTOR_SIZE) != 0)
			{
				nWritePos += QSPI_SECTOR_SIZE - (nWritePos % QSPI_SECTOR_SIZE);
			}
			nCurPos = UpgradeInfo->SectionInfo[i].SectionEnd - nWritePos;
			while (nCurPos < UpgradeInfo->SectionInfo[i].SectionSize)
			{
				flash_write_protection_set(flash_dev, false);
				flash_erase(flash_dev, nWritePos + nCurPos, FLASH_SECTOR_SIZE);
				flash_write_protection_set(flash_dev, true);

				nCurPos += QSPI_SECTOR_SIZE;
			}

			//Update the partition table
			UpgradeInfo->SectionInfo[i].SectionStart = nNextPartitionExpectedStart;
			UpgradeInfo->SectionInfo[i].SectionEnd = UpgradeInfo->SectionInfo[i].SectionStart + UpgradeInfo->SectionInfo[i].SectionSize;

			//Save the memory header section
			LairdFWUpgrade_SaveData(true);
		}

		if (UpgradeInfo->SectionInfo[i].SectionSize > 0)
		{
			nNextPartitionExpectedStart = UpgradeInfo->SectionInfo[i].SectionEnd;
			if ((nNextPartitionExpectedStart % QSPI_SECTOR_SIZE) != 0)
			{
				nNextPartitionExpectedStart += QSPI_SECTOR_SIZE - (nNextPartitionExpectedStart % QSPI_SECTOR_SIZE);
			}
		}
		++i;
	}
}

void LairdFWUpgrade_DebugShowPartitions()
{
	//Debug function that prints the partition information on all valid partitions and the memory
	uint8_t HeaderMatch = strcmp(UpgradeInfo->MagicHeader, MAGIC_HEADER_VALUE);
	LOG_DBG("Magic string: %s (%s)", UpgradeInfo->MagicHeader, (HeaderMatch == 0 ? "Valid" : "Invalid"));
	LOG_DBG("Update version: %d", UpgradeInfo->UpdateVersion);
	LOG_DBG("Boot fails: %d", UpgradeInfo->BootFails);
	LOG_DBG("Force update: %d", UpgradeInfo->ForceUpdate);
	LOG_DBG("Sections present: %d", UpgradeInfo->SectionsPresent);

	uint8_t i = 0;
	while (i < UpgradeInfo->SectionsPresent)
	{
		LOG_DBG("Section %d:\n", i);
		LOG_DBG("\n\tSection Start: 0x%lx\n\tSection End: 0x%lx\n\tSection Size: 0x%lx\n\tTarget: %d\n\tTarget Start: 0x%lx\n\tTarget End: 0x%lx", UpgradeInfo->SectionInfo[i].SectionStart, UpgradeInfo->SectionInfo[i].SectionEnd, UpgradeInfo->SectionInfo[i].SectionSize, UpgradeInfo->SectionInfo[i].Target, UpgradeInfo->SectionInfo[i].TargetStart, UpgradeInfo->SectionInfo[i].TargetEnd);
		LOG_DBG("\n\tTarget Size: 0x%lx\n\tCompressed: %d\n\tVersion: %d\n\tForce %d\n\tChecksum Type: %d\n\tImage Checksum: 0x%lx", UpgradeInfo->SectionInfo[i].TargetSize, UpgradeInfo->SectionInfo[i].Compressed, UpgradeInfo->SectionInfo[i].Version, UpgradeInfo->SectionInfo[i].Force, UpgradeInfo->SectionInfo[i].ChecksumType, UpgradeInfo->SectionInfo[i].ImageChecksum);
		LOG_DBG("\n\tTarget Checksum: 0x%lx\n\tSignature Type: %d", UpgradeInfo->SectionInfo[i].TargetChecksum, UpgradeInfo->SectionInfo[i].SignatureType);
		uint8_t baTmpBuf[SIGNATURE_SIZE * 2 + 1];
		HexEncode(UpgradeInfo->SectionInfo[i].Signature, SIGNATURE_SIZE * 2, baTmpBuf, true, true);
		LOG_DBG("\n\tSignature (hex): %s", log_strdup(baTmpBuf));
#ifdef PARTITION_DATA_FILENAME_REVERSE
		uint8_t a = 0;
		uint8_t b = 0;
		while (b < MAX_FILENAME_SIZE)
		{
			if (UpgradeInfo->SectionInfo[i].Filename[b] == NULL)
			{
				break;
			}
			++b;
		}
		uint8_t c = 0;
		while (a < b)
		{
			c = ((UpgradeInfo->SectionInfo[i].Filename[a] & 0xf) << 4) | ((UpgradeInfo->SectionInfo[i].Filename[a] & 0xf0) >> 4);
			baTmpBuf[a] = c;
			++a;
		}
		baTmpBuf[a] = 0;
#else
		l = 0;
		while (l < MAX_FILENAME_SIZE)
		{
			baTmpBuf[a] = UpgradeInfo->SectionInfo[i].Filename[l];
			++l;
		}
		baTmpBuf[l] = 0;
#endif
		LOG_DBG("\n\tFilename: %s", log_strdup(baTmpBuf));

		bool bPrintData = true;
		a = 0;
		b = MAX_EXTRA_DATA_SIZE;
		while (a < b)
		{
			if (UpgradeInfo->SectionInfo[i].ExtraData[a] > '~' || (UpgradeInfo->SectionInfo[i].ExtraData[a] > NULL && UpgradeInfo->SectionInfo[i].ExtraData[a] < ' ' && UpgradeInfo->SectionInfo[i].ExtraData[a] != '\r' && UpgradeInfo->SectionInfo[i].ExtraData[a] != '\n'))
			{
				bPrintData = false;
			}
			else if (UpgradeInfo->SectionInfo[i].ExtraData[a] == NULL)
			{
				break;
			}
			++a;
		}

		if (bPrintData == true)
		{
			uint8_t nTmpChar;
			if (a == MAX_EXTRA_DATA_SIZE)
			{
				//No null character detected, insert one temporarily
				nTmpChar = UpgradeInfo->SectionInfo[i].ExtraData[MAX_EXTRA_DATA_SIZE-1];
				UpgradeInfo->SectionInfo[i].ExtraData[MAX_EXTRA_DATA_SIZE-1] = NULL;
			}
			LOG_DBG("\n\tExtra data (string): %s", log_strdup(baTmpBuf));
			if (a == MAX_EXTRA_DATA_SIZE)
			{
				//Return character
				UpgradeInfo->SectionInfo[i].ExtraData[MAX_EXTRA_DATA_SIZE-1] = NULL;
			}
		}

		HexEncode(UpgradeInfo->SectionInfo[i].ExtraData, MAX_EXTRA_DATA_SIZE*2, baTmpBuf, true, true);
		LOG_DBG("\n\tExtra data (hex): %s\n\tEncryption key: %d", log_strdup(baTmpBuf), UpgradeInfo->SectionInfo[CurrentPartition].EncryptionKey);
		++i;
	}
}

uint32_t LairdFWUpgrade_Partitions()
{
	return UpgradeInfo->SectionsPresent;
}

int8_t LairdFWUpgrade_IsDataSafe()
{
	//Checks if the QSPI data has been marked as safe 
	uint8_t nResult;
	uint32_t pDataAddr;
	int32_t nDataLen;
	nResult = BootloaderSettings_Get(BOOTLOADER_FUNCTION_INDEX_QSPI_DATA_CHECKED, &pDataAddr, &nDataLen);

	if (nResult != BOOTLOADER_FUNCTION_CODE_SUCCESS)
	{
		//Failed to return success
		return LAIRD_ERROR_CODE_EXTERNAL_SETTINGS_FAILED;
	}

	//
	if (nDataLen != BOOTLOADER_QSPI_HEADER_CHECKED_SIZE)
	{
		//Data length is not valid
		return LAIRD_ERROR_CODE_EXTERNAL_SETTINGS_INVALID_LENGTH;
	}

	//Check the value
	if (*(uint32_t*)pDataAddr == BOOTLOADER_FUNCTION_QSPI_CHECK_CHECKED_ALL)
	{
		//
		return LAIRD_ERROR_CODE_SUCCESS;
	}
	else if (*(uint32_t*)pDataAddr == BOOTLOADER_FUNCTION_QSPI_CHECK_CHECKED_LAIRD)
	{
		//
		return LAIRD_ERROR_CODE_QSPI_DATA_CHECKED_NO_CUSTOMER_KEY;
	}
	else if (*(uint32_t*)pDataAddr == BOOTLOADER_FUNCTION_QSPI_CHECK_NOT_CHECKED)
	{
		//
		return LAIRD_ERROR_CODE_QSPI_DATA_UNCHECKED;
	}

	//Invalid value
	return LAIRD_ERROR_CODE_EXTERNAL_SETTINGS_FAILED;
}

int8_t LairdFWUpgrade_VerifyBootloaderHeader()
{
	//Verifies if the header checksum and hash are correct or not
	uint8_t nResult;
	uint32_t pDataAddr;
	int32_t nDataLen;
	nResult = BootloaderSettings_Get(BOOTLOADER_FUNCTION_INDEX_QSPI_HEADER_CHECKSUM, &pDataAddr, &nDataLen);

	if (nResult != BOOTLOADER_FUNCTION_CODE_SUCCESS)
	{
		//Failed to return success
		return LAIRD_ERROR_CODE_EXTERNAL_SETTINGS_FAILED;
	}

	//
	if (nDataLen != BOOTLOADER_CHECKSUM_SIZE)
	{
		//Data length is not valid
		return LAIRD_ERROR_CODE_EXTERNAL_SETTINGS_INVALID_LENGTH;
	}

	if (GenerateCRC(CHECKSUM_LAIRD_CRC32, UpgradeInfo, sizeof(UpgradeDataStruct), 0x2000 - sizeof(UpgradeDataStruct), 0xff) != *(uint32_t*)pDataAddr)
	{
		//Checksum mismatch
		return LAIRD_ERROR_CODE_CHECKSUM_FAIL;
	}

	nResult = BootloaderSettings_Get(BOOTLOADER_FUNCTION_INDEX_QSPI_HEADER_HASH, &pDataAddr, &nDataLen);
	if (nResult != BOOTLOADER_FUNCTION_CODE_SUCCESS)
	{
		//Failed to return success
		return LAIRD_ERROR_CODE_EXTERNAL_SETTINGS_FAILED;
	}

	//
	if (nDataLen != BOOTLOADER_HASH_SIZE)
	{
		//Data length is not valid
		return LAIRD_ERROR_CODE_EXTERNAL_SETTINGS_INVALID_LENGTH;
	}

	uint8_t baHashBuffer[BOOTLOADER_HASH_SIZE];
	uint8_t baPadding[] = {INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE, INTERNAL_FLASH_PAD_BYTE};
	memset(baHashBuffer, 0x00, sizeof(baHashBuffer));
	struct tc_sha256_state_struct hashState;

	uint32_t nerr = 0;
	nerr = tc_sha256_init(&hashState);
	nerr = tc_sha256_update(&hashState, (uint8_t*)UpgradeInfo, sizeof(UpgradeDataStruct));
	uint16_t nLeft = QSPI_FLASH_BACKUP_HEADER_ADDRESS - sizeof(UpgradeDataStruct);
	while (nLeft > 0)
	{
		nerr = tc_sha256_update(&hashState, baPadding, (nLeft > sizeof(baPadding) ? sizeof(baPadding) : nLeft));
		if (nLeft > sizeof(baPadding))
		{
			nLeft -= sizeof(baPadding);
		}
		else
		{
			nLeft = 0;
		}
	}
	nerr = tc_sha256_final(baHashBuffer, &hashState);

	if (memcmp((uint32_t*)pDataAddr, baHashBuffer, sizeof(baHashBuffer)) != 0)
	{
		//Hash failure
		return LAIRD_ERROR_CODE_HASH_FAIL;
	}

	//Success
	return LAIRD_ERROR_CODE_SUCCESS;
}

