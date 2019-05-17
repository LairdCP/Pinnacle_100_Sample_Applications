/*
 * Copyright (c) 2018 Laird
 * 
 */

#ifndef FOTA_HEADER_H_
#define FOTA_HEADER_H_

#include <zephyr.h>
#include <flash.h>
#include <device.h>
#include <stdio.h>
#include <string.h>
#include "target.h"

#define FLASH_SECTOR_SIZE                           4096
#define MAX_FILENAME_SIZE                           32                             //Size of the optional filename field in the partition data structure (including null byte)
#define MAX_EXTRA_DATA_SIZE                         64                             //Size of the optional extra data field in the partiion data structure (including null byte)
#define SIGNATURE_SIZE                              64
#define MAX_SIGNATURE_SIZE                          128                            //Maximum size (in bytes) that a signature can be - for future planning on systems that may have additional signatures added, this is so that the partition information is sized accordingly

#define PARTITION_DATA_FILENAME
#define PARTITION_DATA_EXTRA_DATA
#define BOOTLOADER_STORAGE_FOR_ENCRYPTION_KEYS
#define PARTITION_DATA_FILENAME_REVERSE                                                //Stores filenames in reverse nibble order (to prevent people reading out the QSPI flash chip and finding the filenames with ease)

#define TRANSFER_SIZE_BYTES                         32                             //Size of transfers (and internal buffers) when reading or writing sections to/from QSPI flash

#define QSPI_SECTOR_SIZE                            0x1000
#define QSPI_PAGE_SIZE                              256 //Page size of QSPI memory when being accessed in SPI mode


//Structures
typedef struct
{
    //This structure holds information on a partition
    uint32_t SectionStart;                   //Address (in QSPI) where this section starts
    uint32_t SectionEnd;                     //Address (in QSPI) where this section ends
    uint32_t SectionSize;                    //Length of section - sanity check should be end-start
    uint32_t TargetStart;                    //Address (in target device) where this section should start writing
    uint32_t TargetEnd;                      //Address (in target device) where this section should stop writing
    uint32_t TargetSize;                     //Length of target - sanity check should be end-start
    uint8_t  Compressed;                     //If the section is compressed or not and the type of compression
    uint8_t  Target;                         //If the target is the nRF52840, or external sequans, etc.
    uint16_t Version;                        //The version of the data area to flash
    bool     Force;                          //If the data should be forcefully flashed even if the versions are the same (i.e. recovery)
    uint8_t  ChecksumType;                   //What type of checksum is used
    uint8_t  XPADDING1[2];                   //Padding for 4-byte boundary, not used
    uint32_t ImageChecksum;                  //A checksum of the data to flash (compressed image)
    uint32_t TargetChecksum;                 //A checksum of the data to flash (when written to target)
    uint8_t  ApplicationType;                //Controls if the application boot position should be updated or if the bootloader should be updated
    uint8_t  SignatureType;                  //What type of signature check is used
    uint8_t  XPADDING2[2];                   //Padding for 4-byte boundary, not used
    uint8_t  Signature[MAX_SIGNATURE_SIZE];  //The embedded signature
#ifdef PARTITION_DATA_FILENAME
    uint8_t  Filename[MAX_FILENAME_SIZE];    //An optional filename field which can be used to aid debugging
#endif
#ifdef PARTITION_DATA_EXTRA_DATA
    uint8_t  ExtraData[MAX_EXTRA_DATA_SIZE]; //An optional extra data field (used to store the version for MODEM updates)
#endif
#ifdef BOOTLOADER_STORAGE_FOR_ENCRYPTION_KEYS
    uint8_t  EncryptionKey;                  //If the data is encryption (for future use, this currently does nothing)
#endif
    uint8_t  XPADDING3[1];                   //Padding for 4-byte boundary, not used

    //Reserved for future use - should all be 0. If you move one above, be sure to reduce/remove from this reserved list as to not mess with existing update file specs, and put your new variable at the bottom of the list of variables just above this line
    uint8_t  RESERVED_8[2];
    uint16_t RESERVED_16[2];
    uint32_t RESERVED_32[2];
} SectionStruct;

typedef struct
{
    //
    char          MagicHeader[sizeof(MAGIC_HEADER_VALUE)];  //A static magic string which is used to check that the data is valid
    uint32_t      Checksum;                                 //CRC32 checksum
    uint8_t       UpdateVersion;                            //Version of the bootloader update data
    uint8_t       BootFails;                                //Number of boot fails
    bool          ForceUpdate;                              //True = force updating all sections
    uint8_t       SectionsPresent;                          //Number of sections present

    //Reserved for future use - should all be 0. If you move one above, be sure to reduce/remove from this reserved list as to not mess with existing update file specs, and put your new variable just above this line, not below the section struct variable
    uint8_t       RESERVED_8[2];
    uint16_t      RESERVED_16[2];
    uint32_t      RESERVED_32[2];

    //
    SectionStruct SectionInfo[SECTIONS_PRESENT];            //Section data
} UpgradeDataStruct;

//Constants
//const int8_t 
#define LAIRD_ERROR_CODE_SUCCESS                            0
#define LAIRD_ERROR_CODE_ALL_SECTIONS_USED                 -1
#define LAIRD_ERROR_CODE_PARTITION_ALREADY_OPEN            -2
#define LAIRD_ERROR_CODE_PREVIOUS_PARTITION_DATA_INVALID   -3
#define LAIRD_ERROR_CODE_PARTITION_NOT_OPEN                -4
#define LAIRD_ERROR_CODE_PARTITION_NOT_VALID               -5
#define LAIRD_ERROR_CODE_PARTITION_NOT_CREATED             -6
#define LAIRD_ERROR_CODE_NO_PARTITIONS                     -7
#define LAIRD_ERROR_CODE_PARTITION_IN_USE                  -8
#define LAIRD_ERROR_CODE_FLASH_INIT_FAILED                 -9
#define LAIRD_ERROR_CODE_INVALID_DATA                      -10
#define LAIRD_ERROR_CODE_CHECKSUM_FAIL                     -11
#define LAIRD_ERROR_CODE_EXTERNAL_SETTINGS_FAILED          -12
#define LAIRD_ERROR_CODE_EXTERNAL_SETTINGS_INVALID_LENGTH  -13
#define LAIRD_ERROR_CODE_QSPI_DATA_UNCHECKED               -14
#define LAIRD_ERROR_CODE_QSPI_DATA_CHECKED_NO_CUSTOMER_KEY -15
#define LAIRD_ERROR_CODE_HASH_FAIL                         -16

#define LAIRD_ERROR_CODE_NOT_IMPLEMENTED                   -50

#define PARTITION_OPTIONS_NULL_SIZE      0b1
#define PARTITION_OPTIONS_CHECK_CHECKSUM 0b10


//Function prototypes
int32_t LairdFWUpgrade_Init();
int8_t LairdFWUpgrade_Destroy();
int32_t LairdFWUpgrade_LoadData();
int32_t LairdFWUpgrade_SaveData(bool bSaveBackup);
void LairdFWUpgrade_CreateEmpty();
SectionStruct *LairdFWUpgrade_GetPartitionData(uint8_t nPartition);
int8_t LairdFWUpgrade_NextFreePartition();
int8_t LairdFWUpgrade_ClearAll();
int8_t LairdFWUpgrade_OpenPartition(uint8_t unPartition);
int8_t LairdFWUpgrade_IsPartitionOpen();
int8_t LairdFWUpgrade_PartitionOpenNumber();
int8_t LairdFWUpgrade_ClosePartition(int8_t unOptions);
int32_t LairdFWUpgrade_WritePartition(uint8_t *Data, uint32_t DataSize);
int8_t LairdFWUpgrade_UsedPartitions();
int8_t LairdFWUpgrade_FreePartitions();
int32_t LairdFWUpgrade_UsedPartitionSpace(uint8_t Partition);
int32_t LairdFWUpgrade_TotalUsedSpaceNoGaps();
int32_t LairdFWUpgrade_TotalUsedSpace();
int32_t LairdFWUpgrade_TotalFreeSpace();
int32_t LairdFWUpgrade_TotalSpace();
int8_t LairdFWUpgrade_DeletePartition(uint8_t unPartition);
void LairdFWUpgrade_Defragment();
uint32_t LairdFWUpgrade_Partitions();
int8_t LairdFWUpgrade_IsDataSafe();
int8_t LairdFWUpgrade_VerifyBootloaderHeader();

//Debug function
void LairdFWUpgrade_DebugShowPartitions();

#endif
