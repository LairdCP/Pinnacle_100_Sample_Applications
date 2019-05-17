#ifndef TARGET_HEADER_H_
#define TARGET_HEADER_H_

#include <stdint.h>
#include <stdbool.h>

enum CHECKSUM_TYPES
{
    CHECKSUM_NORDIC_CRC16                             = 0,                             //If a checksum of a section should use Nordic's CRC-16
    CHECKSUM_NORDIC_CRC32,                                                             //If a checksum of a section should use Nordic's CRC-32
    RESERVED,                                                                          //Reserved for future use (Laird CRC-16)
    CHECKSUM_LAIRD_CRC32,                                                              //If a checksum of a section should use Laird's CRC-32
    CHECKSUM_BYPASS                                   = 254                            //If a checksum of a section should be bypassed
};

#define INTERNAL_FLASH_PAD_BYTE 0xff

#define QSPI_FLASH_SIZE 0x100000
#define QSPI_FLASH_PAD_BYTE 0xff

#define QSPI_FLASH_MAIN_HEADER_ADDRESS                  0x0                            //Address of main header in QSPI flash
#define QSPI_FLASH_BACKUP_HEADER_ADDRESS                0x2000                         //Address of backup header in QSPI flash
#define QSPI_FLASH_FIRST_PARTITION_ADDRESS              0x4000                         //Address of first partition segment in QSPI flash
#define SECTIONS_PRESENT                                24                             //Number of sections that can be present in the QSPI flash in total

#define QSPI_FLASH_LABEL                                "MX25R64"

#define LAIRD_FW_DEBUG                                  0

#define MAGIC_HEADER_VALUE                              "Laird Connectivity Sirrus 100 Bootloader (C) 2019. Do not distribute/edit/decompile/analyse/alter this data."

#define QSPI_MIN_WRITE_SIZE                             4

#define BOOTLOADER_FUNCTION_INDEX_QSPI_DATA_CHECKED     0x0a
#define BOOTLOADER_FUNCTION_INDEX_QSPI_HEADER_CHECKSUM  0x0b
#define BOOTLOADER_FUNCTION_INDEX_QSPI_HEADER_HASH      0x0c
#define BOOTLOADER_FUNCTION_INDEX_VERSION_START         0x20                           //Space for 32 slots
#define BOOTLOADER_FUNCTION_INDEX_CODE_VERSION          0xff

#define BOOTLOADER_FUNCTION_CODE_SUCCESS                0
#define BOOTLOADER_FUNCTION_CODE_INVALID_ID             5
#define BOOTLOADER_FUNCTION_CODE_NOT_PRESENT            6
#define BOOTLOADER_FUNCTION_CODE_DISCREPANCY_DETECTED   7

//Values for what the QSPI value checked means
#define BOOTLOADER_FUNCTION_QSPI_CHECK_NOT_CHECKED      0                              //QSPI contents not checked e.g. no license
#define BOOTLOADER_FUNCTION_QSPI_CHECK_CHECKED_LAIRD    1                              //QSPI Laird key contents checked only
#define BOOTLOADER_FUNCTION_QSPI_CHECK_CHECKED_ALL      2                              //QSPI full contents checked (including customer key sections)

#define BOOTLOADER_QSPI_HEADER_CHECKED_SIZE             1
#define BOOTLOADER_CHECKSUM_SIZE                        4
#define BOOTLOADER_HASH_SIZE                            32

//Bootloader function for getting variables
#define BOOTLOADER_STORAGE_FUNCTION_GET_ADDRESS         0xfc001                        //Address of function for loading settings from bootloader
typedef int (*bootloader_function_get_t)(uint32_t nIndex, uint32_t *pAddress, uint32_t *pSize, uint16_t *pFlags, uint8_t RFU_IN, uint8_t *RFU_OUT);

#endif
