/******************************************************************************
**              Copyright (C) 2019 Laird Connectivity
**
** Project:     Pinnacle 100 Bootloader
**
** Module:      Bootloader_External_Settings.h
**
** Notes:
*******************************************************************************/
#ifndef BOOTLOADER_EXTERNAL_SETTINGS_H__
#define BOOTLOADER_EXTERNAL_SETTINGS_H__

/******************************************************************************/
/* Include Files*/
/******************************************************************************/
//#include "target.h"
#define SIGNATURE_SIZE                                  64                             //Size (in bytes) of a signature


/******************************************************************************/
/* External Variable Declarations                                             */
/******************************************************************************/

//From linker: start address and size (not size of function itself) of bootloader storage section
//extern uint32_t __bootloader_storage_lookup_start__[];
//extern uint32_t __bootloader_storage_lookup_size__[];

/******************************************************************************/
/* Local Defines*/
/******************************************************************************/

#define BOOTLOADER_STORAGE_HEADER_INFO_VERSION                        1             //Version of the information struct header
#define BOOTLOADER_STORAGE_CODE_VERSION                               1             //Version of this part of the code

#define BL_INFO_HEADER_MAX_SIZE                                       320           //Total size of the information struct (including padding)
#define BL_INFO_HEADER_PADDING                                        52            //Padding of information struct, should be above value minus actual used size

//Indexes of WORM variables in the bootloader
#define BOOTLOADER_STORAGE_INDEX_LICENSE                              0x00000000
#define BOOTLOADER_STORAGE_INDEX_BT_ADDR                              0x00000001
#define BOOTLOADER_STORAGE_INDEX_CUSTOMER_PK_SET                      0x00000002
#define BOOTLOADER_STORAGE_INDEX_CUSTOMER_PK                          0x00000003
#define BOOTLOADER_STORAGE_INDEX_READBACK_PROTECTION                  0x00000004
#define BOOTLOADER_STORAGE_INDEX_CPU_DEBUG_PROTECTION                 0x00000005
#define BOOTLOADER_STORAGE_INDEX_BLOCK_UART_BLDR_READBACK             0x00000006
#define BOOTLOADER_STORAGE_INDEX_QSPI_MODE                            0x00000007
#define BOOTLOADER_STORAGE_INDEX_QSPI_CHECKED                         0x0000000a    //If the QSPI contents have been checked by the bootloader e.g. license is valid
#define BOOTLOADER_STORAGE_INDEX_QSPI_HEADER_CRC                      0x0000000b    //Checksum of the QSPI header contents, for checking it hasn't been altered
#define BOOTLOADER_STORAGE_INDEX_QSPI_HEADER_SHA256                   0x0000000c    //SHA256 hash of the QSPI header contents, for checking it hasn't been altered
#define BOOTLOADER_STORAGE_INDEX_QSPI_CUSTOMER_SPACE_START_OFFSET     0x0000000d    //Starting QSPI sectors for use by customers
#define BOOTLOADER_STORAGE_INDEX_QSPI_CUSTOMER_SPACE_START_SECTOR     0x0000000e    //
#define BOOTLOADER_STORAGE_INDEX_QSPI_CUSTOMER_SPACE_SECTORS          0x0000000f    //
#define BOOTLOADER_STORAGE_INDEX_QSPI_CUSTOMER_SPACE_SECTOR_SIZE      0x00000010    //
#define BOOTLOADER_STORAGE_INDEX_SECTION_VERSION                      0x00000020
#ifdef BOOTLOADER_STORAGE_FOR_CUSTOMER_WORM
#define BOOTLOADER_STORAGE_INDEX_CUSTOMER_REG32                       0x00000041
#define BOOTLOADER_STORAGE_INDEX_CUSTOMER_REG64                       0x00000042
#define BOOTLOADER_STORAGE_INDEX_CUSTOMER_REG128                      0x00000043
#endif

//For DFU queries over UART
#define BOOTLOADER_STORAGE_INDEX_CODE_FUNCTION_VERSION_DFU_QUERY      0xff
#define BOOTLOADER_STORAGE_INDEX_CODE_STRUCT_VERSION_DFU_QUERY        0xfe

//
#define BOOTLOADER_STORAGE_INDEX_CHECKSUM_DFU_QUERY                   0x00
#define BOOTLOADER_STORAGE_INDEX_FUNCTION_ADDRESS_DFU_QUERY           0x01
#define BOOTLOADER_STORAGE_INDEX_HEADER_VERSION_DFU_QUERY             0x02
#define BOOTLOADER_STORAGE_INDEX_BUILD_DATE_DFU_QUERY                 0x03
#define BOOTLOADER_STORAGE_INDEX_HEADER_SIZE_DFU_QUERY                0x04
#define BOOTLOADER_STORAGE_INDEX_CHECKSUM_TYPE_DFU_QUERY              0x05
#define BOOTLOADER_STORAGE_INDEX_AREAS_DFU_QUERY                      0x06
#define BOOTLOADER_STORAGE_INDEX_AREA_INFO_START_DFU_QUERY            0x40
#define BOOTLOADER_STORAGE_INDEX_AREA_INFO_ADDRESS_DFU_QUERY          0x00
#define BOOTLOADER_STORAGE_INDEX_AREA_INFO_SIZE_DFU_QUERY             0x01
#define BOOTLOADER_STORAGE_INDEX_AREA_INFO_CHECKSUM_DFU_QUERY         0x02
#define BOOTLOADER_STORAGE_INDEX_AREA_INFO_VERSION_DFU_QUERY          0x03
#define BOOTLOADER_STORAGE_INDEX_AREA_INFO_TYPE_DFU_QUERY             0x04
#define BOOTLOADER_STORAGE_INDEX_AREA_INFO_ACCESS_DFU_QUERY           0x05
#define BOOTLOADER_STORAGE_INDEX_AREA_INFO_SIGNATURE_TYPE_DFU_QUERY   0x06
#define BOOTLOADER_STORAGE_INDEX_AREA_INFO_SIGNATURE_DFU_QUERY        0x07
#define BOOTLOADER_STORAGE_AREA_INFO_PARTITION_BITS                   0xf0
#define BOOTLOADER_STORAGE_AREA_INFO_PARTITION_SHIFT_BITS             4
#define BOOTLOADER_STORAGE_AREA_INFO_QUERY_BITS                       0x0f

//Result codes for the bootloader storage retrieval function
enum BOOTLOADER_STORAGE_CODES
{
    BOOTLOADER_STORAGE_CODE_SUCCESS                                   = 0,
    BOOTLOADER_STORAGE_CODE_INVALID_ID                                = 5,
    BOOTLOADER_STORAGE_CODE_NOT_PRESENT                               = 6,
    BOOTLOADER_STORAGE_CODE_DISCREPANCY_DETECTED                      = 7,
    BOOTLOADER_STORAGE_CODE_INVALID_PARTITION                         = 8,
    BOOTLOADER_STORAGE_CODE_INVALID_SUB_ID                            = 9,
};

//Types of variables returned
#define BOOTLOADER_STORAGE_FLAG_NUMERIC_LITTLE_ENDIAN                 0b1           //Little endian number
#define BOOTLOADER_STORAGE_FLAG_NUMERIC_BIG_ENDIAN                    0b10          //Big endian number (not used)
#define BOOTLOADER_STORAGE_FLAG_BYTE_ARRAY                            0b100         //Byte array
#define BOOTLOADER_STORAGE_FLAG_BYTE_ARRAY_16                         0b1000        //Byte array but 16-bit shifts, basically used for license
#define BOOTLOADER_STORAGE_FLAG_STRING_NOT_TERMINATED                 0b10000       //String without any termination character (not used)
#define BOOTLOADER_STORAGE_FLAG_STRING_NULL_TERMINATED                0b100000      //String with null termination character (not used)

//Start address of where the bootloader settings are stored
#define BOOTLOADER_STORAGE_MEMORY_START_ADDR                          BOOTLOADER_BACKUP_SETTINGS_ADDRESS
#define BOOTLOADER_EXTERNAL_STRUCT_ADDR                               0xfcec0

/******************************************************************************/
/* Local Forward Class,Struct & Union Declarations*/
/******************************************************************************/

enum BL_INFO_SECTION_TYPES
{
    BL_INFO_SECTION_TYPE_MBR = 0,
    BL_INFO_SECTION_TYPE_BOOTLOADER,
    BL_INFO_SECTION_TYPE_EXTERNAL_FUNCTION,

    BL_INFO_SECTION_TYPE_MAX
};

enum BL_INFO_ACCESS_TYPES
{
    BL_INFO_ACCESS_TYPE_NONE = 0,
    BL_INFO_ACCESS_TYPE_READ = 1,
    BL_INFO_ACCESS_TYPE_WRITE = 2,
    BL_INFO_ACCESS_TYPE_EXECUTE = 4
};

enum BL_INFO_SIGNATURE_TYPES
{
    BL_INFO_SIGNATURE_TYPE_INVALID = 0,
    BL_INFO_SIGNATURE_TYPE_BOOTLOADER_KEY
};

enum BL_INFO_CHECKSUM_TYPES
{
    BL_INFO_CHECKSUM_TYPE_NORDIC_CRC16 = 0,
    BL_INFO_CHECKSUM_TYPE_NORDIC_CRC32,
    BL_INFO_CHECKSUM_TYPE_RESERVED,
    BL_INFO_CHECKSUM_TYPE_LAIRD_CRC32
};

//This structure holds information on an area
typedef struct
{
    uint32_t                 StartAddress;                           //Start address of area
    uint32_t                 Size;                                   //Size of area
    uint32_t                 Checksum;                               //CRC32 of area
    uint16_t                 Version;                                //Version of area
    uint8_t                  Type : 3;                               //Type of area
    uint8_t                  Access : 3;                             //Access e.g. if read access is allowed after bootloader has finished booting
    uint8_t                  SignatureType : 2;                      //Type of signature
    uint8_t                  Signature[SIGNATURE_SIZE];              //Signature of area

} BootloaderAreaInfoStruct;

//This structure holds information on the external settings function
typedef struct
{
    uint32_t                 Checksum;                               //CRC32 of this data
    uint32_t                 ExternalFunctionAddress;                //Address of function
    uint16_t                 HeaderVersion;                          //Version of this data
    uint16_t                 ExternalFunctionVersion;                //Version of function
    uint16_t                 HeaderSize;                             //Size of data in this header
    char                     BuildDate[sizeof(__DATE__)];            //Build date
    uint8_t                  ChecksumType;                           //Type of checksums used in this header
    uint8_t                  Areas;                                  //Number of areas in this section
    BootloaderAreaInfoStruct AreaInfo[BL_INFO_SECTION_TYPE_MAX];     //Information on each section

    uint8_t                  RESERVED_8[BL_INFO_HEADER_PADDING];     //For future use, padding for a total of 384 bytes
} BootloaderExternalSettingsInfoStruct;

/******************************************************************************/
/* Local Functions or Private Members*/
/******************************************************************************/

/*=============================================================================*/
// Typedef for external settings function for external applications
/*=============================================================================*/
//typedef uint8_t (*bootloader_function_setting_t)(uint32_t nIndex, uint8_t nPartition, uint8_t nSubKey, uint8_t *pBuffer, uint32_t nBufferSize, uint32_t *pFullSettingSize, uint16_t *pFlags);

/*=============================================================================*/
// Returns data from the bootloader settings area
/*=============================================================================*/
//__attribute__((section(".bootloader_storage_lookup"))) __attribute__((used))
//uint8_t
//GetBootloaderSetting(
//    uint32_t nIndex,
//    uint8_t nPartition,
//    uint8_t nSubKey,
//    uint8_t *pBuffer,
//    uint32_t nBufferSize,
//    uint32_t *pFullSettingSize,
//    uint16_t *pFlags
//    );

/*=============================================================================*/
/* External Bootloader function which is used to do a memcpy without external  */
/* function calls to code that might be read/execute protected                 */
/*=============================================================================*/
//__attribute__((section(".bootloader_storage_lookup"))) __attribute__((used))
//static
//void
//ExternalMemCpy(
//    void *pDest,
//    void *pSrc,
//    uint32_t nSize
//    );

/*=============================================================================*/
/* External Bootloader function which is used to do a memory erase check       */
/* without external function calls to code that might be read/execute          */
/* protected                                                                   */
/*=============================================================================*/
//__attribute__((section(".bootloader_storage_lookup"))) __attribute__((used))
//static
//bool
//ExternalCheckIfMemoryRegionErased(
//    uint8_t *pAddr,
//    uint8_t nLen
//    );

//extern __attribute__((section(".bootloader_storage_lookup_struct"))) __attribute__((used)) BootloaderExternalSettingsInfoStruct pBootloaderExternalSettingsInfo;

#endif
/******************************************************************************/
/* END OF FILE*/
/******************************************************************************/
