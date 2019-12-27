/* main.c - Application main entry point */

#include <zephyr.h>
#include <misc/printk.h>
#include <version.h>
#include <gpio.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <stdio.h>
#include "nrf.h"
#include "nrf_power.h"
#include "Bootloader_External_Settings.h"

#define GPREGRET_BOOTLOADER_UART_START_VALUE       0xb1
#define MAX_DATA_SIZE                              140
#define DISPLAY_NO_DATA                            '1'
#define DISPLAY_ASCII                              '2'
#define DISPLAY_HEX_BYTE_ENDIAN_FLIP               '3'
#define DISPLAY_HEX_QUAD_ENDIAN_FLIP               '4'

/* BOOTLOADER */
#define BOOTLOADER_STORAGE_FUNCTION_GET_ADDRESS    0xfc02b /* Address of function for loading settings from bootloader */
typedef int (*bootloader_function_get_t)(uint32_t nIndex, uint8_t nPartition, uint8_t nSubKey, uint8_t *pBuffer, uint32_t nBufferSize, uint32_t *pFullSettingSize, uint16_t *pFlags); /* Bootloader function for getting variables */

BootloaderExternalSettingsInfoStruct *pBootloaderExternalSettingsInfo;

static int ubldr_boot(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    //Enter UART bootloader: clear the current general purpose register and set it to the UART bootloader value
    nrf_power_gpregret_set(GPREGRET_BOOTLOADER_UART_START_VALUE);
    NVIC_SystemReset();

    return 0;
}

static int ubldr_get(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);

    const u8_t *cmd = argv[1];

    uint32_t nCmdID = strtoul(cmd, NULL, 16);

    bootloader_function_get_t bldrload = (bootloader_function_get_t)BOOTLOADER_STORAGE_FUNCTION_GET_ADDRESS;

    uint16_t nFlags = 0;
    uint32_t nDataLen = 0;
    uint32_t resCode;
    resCode = bldrload(nCmdID, 0, 0, NULL, 0, &nDataLen, &nFlags);

    printk("  Ret: %d, Len: %d, Flg: %x, ", resCode, nDataLen, nFlags);
    if (argc > 2)
    {
        const u8_t *cmd = argv[2];
        if (cmd[0] == DISPLAY_NO_DATA)
        {
            printk("\r\n");
            return 0;
        }
    }

    if (resCode == 0)
    {
        uint8_t pData[nDataLen+1];
        uint8_t pDataDupe[(nDataLen*2)+1];
        resCode = bldrload(nCmdID, 0, 0, pData, nDataLen, NULL, NULL);

        pDataDupe[0] = 0;
        if (pData != NULL)
        {
            uint8_t i = 0;
            if (argc > 2)
            {
                const u8_t *cmd = argv[2];
                if (cmd[0] == DISPLAY_ASCII)
                {
                    printk("Data: %s", pData);
                }
                else
                {
                    while (i < nDataLen)
                    {
                        if (cmd[0] == DISPLAY_HEX_BYTE_ENDIAN_FLIP)
                        {
                            sprintf(pDataDupe, "%s%02X%02X", pDataDupe, pData[i+1], pData[i]);
                        }
                        else if (cmd[0] == DISPLAY_HEX_QUAD_ENDIAN_FLIP)
                        {
                            sprintf(pDataDupe, "%s%02X%02X", pDataDupe, pData[nDataLen-i-1], pData[nDataLen-i-2]);
                        }
                        else
                        {
                            sprintf(pDataDupe, "%s%02X%02X", pDataDupe, pData[i], pData[i+1]);
                        }

                        i += 2;
                    }
                    printk("Data: %s", pDataDupe);
                }
            }
            else
            {
                while (i < nDataLen)
                {
                    sprintf(pDataDupe, "%s%02X%02X", pDataDupe, pData[i], pData[i+1]);
                    i += 2;
                }
                printk("Data: %s", pDataDupe);
            }
        }
        else
        {
            printk("failed.");
        }
    }
    printk("  \r\n");

    return 0;
}

static int ubldr_getb(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);

    const u8_t *cmd = argv[1];
    u8_t nPartition = strtoul(argv[2], NULL, 16);

    uint32_t nCmdID = strtoul(cmd, NULL, 16);

    bootloader_function_get_t bldrload = (bootloader_function_get_t)BOOTLOADER_STORAGE_FUNCTION_GET_ADDRESS;

    uint16_t nFlags = 0;
    uint32_t nDataLen = 0;
    uint32_t resCode;
    resCode = bldrload(nCmdID, nPartition, 0, NULL, 0, &nDataLen, &nFlags);

    printk("  Partition: %d, Ret: %d, Len: %d, Flg: %x, ", nPartition, resCode, nDataLen, nFlags);
    if (argc > 3)
    {
        const u8_t *cmd = argv[3];
        if (cmd[0] == DISPLAY_NO_DATA)
        {
            printk("\r\n");
            return 0;
        }
    }

    if (resCode == 0)
    {
        uint8_t pData[nDataLen+1];
        uint8_t pDataDupe[(nDataLen*2)+1];
        resCode = bldrload(nCmdID, nPartition, 0, pData, nDataLen, NULL, NULL);

        pDataDupe[0] = 0;
        if (pData != NULL)
        {
            uint8_t i = 0;
            if (argc > 3)
            {
                const u8_t *cmd = argv[3];
                if (cmd[0] == DISPLAY_ASCII)
                {
                    printk("Data: %s", pData);
                }
                else
                {
                    while (i < nDataLen)
                    {
                        if (cmd[0] == DISPLAY_HEX_BYTE_ENDIAN_FLIP)
                        {
                            sprintf(pDataDupe, "%s%02X%02X", pDataDupe, pData[i+1], pData[i]);
                        }
                        else if (cmd[0] == DISPLAY_HEX_QUAD_ENDIAN_FLIP)
                        {
                            sprintf(pDataDupe, "%s%02X%02X", pDataDupe, pData[nDataLen-i-1], pData[nDataLen-i-2]);
                        }
                        else
                        {
                            sprintf(pDataDupe, "%s%02X%02X", pDataDupe, pData[i], pData[i+1]);
                        }

                        i += 2;
                    }
                    printk("Data: %s", pDataDupe);
                }
            }
            else
            {
                while (i < nDataLen)
                {
                    sprintf(pDataDupe, "%s%02X%02X", pDataDupe, pData[i], pData[i+1]);
                    i += 2;
                }
                printk("Data: %s", pDataDupe);
            }
        }
        else
        {
            printk("failed.");
        }
    }
    printk("  \r\n");

    return 0;
}

static int ubldr_getinfo(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);

    const u8_t *cmd = argv[1];
    u8_t nPartition = strtoul(argv[2], NULL, 16);

    uint32_t nCmdID = strtoul(cmd, NULL, 16);
    void *pDataAddr = NULL;
    uint32_t nDataSize = 0;
    uint8_t nTmpVal;
    uint8_t nOutputType;

//0 = numeric, decimal
//1 = numeric, hex
//2 = numeric, hex, 8 characters (32 bits)
//3 = byte array
//4 = string

    if (nCmdID == 0)
    {
        //Checksum
        pDataAddr = &pBootloaderExternalSettingsInfo->Checksum;
        nDataSize = sizeof(pBootloaderExternalSettingsInfo->Checksum);
        nOutputType = 2;
    }
    else if (nCmdID == 1)
    {
        //External function address
        pDataAddr = &pBootloaderExternalSettingsInfo->ExternalFunctionAddress;
        nDataSize = sizeof(pBootloaderExternalSettingsInfo->ExternalFunctionAddress);
        nOutputType = 2;
    }
    else if (nCmdID == 2)
    {
        //Header version
        pDataAddr = &pBootloaderExternalSettingsInfo->HeaderVersion;
        nDataSize = sizeof(pBootloaderExternalSettingsInfo->HeaderVersion);
        nOutputType = 0;
    }
    else if (nCmdID == 3)
    {
        //External function version
        pDataAddr = &pBootloaderExternalSettingsInfo->ExternalFunctionVersion;
        nDataSize = sizeof(pBootloaderExternalSettingsInfo->ExternalFunctionVersion);
        nOutputType = 0;
    }
    else if (nCmdID == 4)
    {
        //Header size
        pDataAddr = &pBootloaderExternalSettingsInfo->HeaderSize;
        nDataSize = sizeof(pBootloaderExternalSettingsInfo->HeaderSize);
        nOutputType = 2;
    }
    else if (nCmdID == 5)
    {
        //Build date
        pDataAddr = pBootloaderExternalSettingsInfo->BuildDate;
        nDataSize = sizeof(sizeof(__DATE__));
        nOutputType = 4;
    }
    else if (nCmdID == 6)
    {
        //Checksum type
        pDataAddr = &pBootloaderExternalSettingsInfo->ChecksumType;
        nDataSize = sizeof(pBootloaderExternalSettingsInfo->ChecksumType);
        nOutputType = 1;
    }
    else if (nCmdID == 5)
    {
        //Areas
        pDataAddr = &pBootloaderExternalSettingsInfo->Areas;
        nDataSize = sizeof(pBootloaderExternalSettingsInfo->Areas);
        nOutputType = 0;
    }

//NOTE: following will blindly use array ID even if it is not valid - regression testing ONLY!

    else if (nCmdID == 0x40)
    {
        //Area: Start address
        pDataAddr = &pBootloaderExternalSettingsInfo->AreaInfo[nPartition].StartAddress;
        nDataSize = sizeof(pBootloaderExternalSettingsInfo->AreaInfo[0].StartAddress);
        nOutputType = 2;
    }
    else if (nCmdID == 0x41)
    {
        //Area: Size
        pDataAddr = &pBootloaderExternalSettingsInfo->AreaInfo[nPartition].Size;
        nDataSize = sizeof(pBootloaderExternalSettingsInfo->AreaInfo[0].Size);
        nOutputType = 2;
    }
    else if (nCmdID == 0x42)
    {
        //Area: Checksum
        pDataAddr = &pBootloaderExternalSettingsInfo->AreaInfo[nPartition].Checksum;
        nDataSize = sizeof(pBootloaderExternalSettingsInfo->AreaInfo[0].Checksum);
        nOutputType = 2;
    }
    else if (nCmdID == 0x43)
    {
        //Area: Version
        pDataAddr = &pBootloaderExternalSettingsInfo->AreaInfo[nPartition].Version;
        nDataSize = sizeof(pBootloaderExternalSettingsInfo->AreaInfo[0].Version);
        nOutputType = 0;
    }
    else if (nCmdID == 0x44)
    {
        //Area: Type
        nTmpVal = pBootloaderExternalSettingsInfo->AreaInfo[nPartition].Type;
        pDataAddr = &nTmpVal;
        nDataSize = sizeof(nTmpVal);
        nOutputType = 0;
    }
    else if (nCmdID == 0x45)
    {
        //Area: Access
        nTmpVal = pBootloaderExternalSettingsInfo->AreaInfo[nPartition].Access;
        pDataAddr = &nTmpVal;
        nDataSize = sizeof(nTmpVal);
        nOutputType = 0;
    }
    else if (nCmdID == 0x46)
    {
        //Area: Signature type
        nTmpVal = pBootloaderExternalSettingsInfo->AreaInfo[nPartition].SignatureType;
        pDataAddr = &nTmpVal;
        nDataSize = sizeof(nTmpVal);
        nOutputType = 0;
    }
    else if (nCmdID == 0x47)
    {
        //Area: Signature
        pDataAddr = pBootloaderExternalSettingsInfo->AreaInfo[nPartition].Signature;
        nDataSize = SIGNATURE_SIZE;
        nOutputType = 3;
    }

    if (pDataAddr == NULL || nDataSize == 0)
    {
        //Nothing to output
        printk("  Cmd: %d, Area: %d, Ret: 1  \r\n", nCmdID, nPartition);
    }
    else
    {
        //Something to output
        printk("  Cmd: %d, Area: %d, Ret: 0, Len: %d, Data: ", nCmdID, nPartition, nDataSize);
        if (nOutputType == 0)
        {
            //Numeric, decimal
            if (nDataSize == 1)
            {
                printk("%d", *(uint8_t*)pDataAddr);
            }
            else if (nDataSize == 2)
            {
                printk("%d", *(uint16_t*)pDataAddr);
            }
            else
            {
                printk("%d", *(uint32_t*)pDataAddr);
            }
        }
        else if (nOutputType == 1)
        {
            //Numeric, hex
            if (nDataSize == 1)
            {
                printk("%x", *(uint8_t*)pDataAddr);
            }
            else if (nDataSize == 2)
            {
                printk("%x", *(uint16_t*)pDataAddr);
            }
            else
            {
                printk("%x", *(uint32_t*)pDataAddr);
            }
        }
        else if (nOutputType == 2)
        {
            //Numeric, hex, 8 characters (32 bit)
            if (nDataSize == 1)
            {
                printk("%08x", *(uint8_t*)pDataAddr);
            }
            else if (nDataSize == 2)
            {
                printk("%08x", *(uint16_t*)pDataAddr);
            }
            else
            {
                printk("%08x", *(uint32_t*)pDataAddr);
            }
        }
        else if (nOutputType == 3)
        {
            //Byte array
            uint8_t pDataDupe[(nDataSize*2)+1];
            uint32_t i = 0;
            pDataDupe[0] = 0;
            while (i < nDataSize)
            {
                sprintf(pDataDupe, "%s%02X%02X", pDataDupe, ((uint8_t*) pDataAddr)[i], ((uint8_t*)pDataAddr)[i+1]);
                i += 2;
            }
            printk("%s", pDataDupe);
        }
        else if (nOutputType == 4)
        {
            //String
            printk("%s", pDataAddr);
        }
    }

    printk("  \r\n");

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    ubldr_cmds, SHELL_CMD(boot, NULL, "Boot into universal bootloader", ubldr_boot),
    SHELL_CMD_ARG(get, NULL, "Get universal bootloader info", ubldr_get,
                  2, 1),
    SHELL_CMD_ARG(getb, NULL, "Get extended universal bootloader info", ubldr_getb,
                  3, 1),
    SHELL_CMD_ARG(getinfo, NULL, "Get universal bootloader struct info", ubldr_getinfo,
                  2, 1),
    SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(ubldr, &ubldr_cmds, "Universal Bootloader commands", NULL);


void main(void)
{
    printk("Universal Bootloader Regression Test\r\nBranch: %s\r\nCommit: %s\r\nSystem up time %d us\r\n\r\n",
           GIT_BRANCH, GIT_COMMIT_HASH,
           SYS_CLOCK_HW_CYCLES_TO_NS(k_cycle_get_32()) / 1000);

    //Create sturct object at correct address
    pBootloaderExternalSettingsInfo = (uint32_t*)BOOTLOADER_EXTERNAL_STRUCT_ADDR;
}
