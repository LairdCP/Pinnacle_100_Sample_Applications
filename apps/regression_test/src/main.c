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

#define GPREGRET_BOOTLOADER_UART_START_VALUE       0xb1
#define MAX_DATA_SIZE                              140
#define DISPLAY_NO_DATA                            '1'
#define DISPLAY_ASCII                              '2'
#define DISPLAY_HEX_BYTE_ENDIAN_FLIP               '3'
#define DISPLAY_HEX_QUAD_ENDIAN_FLIP               '4'

/* BOOTLOADER */
#define BOOTLOADER_STORAGE_FUNCTION_GET_ADDRESS    0xfc001 /* Address of function for loading settings from bootloader */
#define BOOTLOADER_STORAGE_INDEX_LICENSE           0 /* Index of license WORM variable in the bootloader */
#define BOOTLOADER_STORAGE_INDEX_BT_ADDR           1 /* Index of BT address WORM variable in the bootloader */
typedef int (*bootloader_function_get_t)(uint32_t nIndex, uint32_t *pAddress, uint32_t *pSize, uint16_t *pFlags, uint8_t RFU_IN, uint8_t *RFU_OUT); /* Bootloader function for getting variables */

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

	uint32_t nCmdID = strtol(cmd, NULL, 16);

	bootloader_function_get_t bldrload = (bootloader_function_get_t)BOOTLOADER_STORAGE_FUNCTION_GET_ADDRESS;
	uint16_t nFlags;
	uint32_t resCode, nDataAddr, nDataLen;
	resCode = bldrload(nCmdID, &nDataAddr, &nDataLen, &nFlags, NULL, NULL);

	printk("Ret: %d, Pos: %d, Len: %d, Flg: %x, ", resCode, nDataAddr, nDataLen, nFlags);
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
		uint8_t pData[MAX_DATA_SIZE];
		uint8_t pDataDupe[MAX_DATA_SIZE*2];
		pDataDupe[0] = 0;
		if (pData != NULL)
		{
			memcpy(pData, (uint32_t*)nDataAddr, nDataLen);
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
	printk("\r\n");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	ubldr_cmds, SHELL_CMD(boot, NULL, "Boot into universal bootloader", ubldr_boot),
	SHELL_CMD_ARG(get, NULL, "Get universal bootloader info", ubldr_get,
		      2, 1),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(ubldr, &ubldr_cmds, "Universal Bootloader commands", NULL);


void main(void)
{
	printk("Pinnacle 100 Bootloader Regression Test\r\nBranch: %s\r\nCommit: %s\r\nSystem up time %d us\r\n\r\n",
	       GIT_BRANCH, GIT_COMMIT_HASH,
	       SYS_CLOCK_HW_CYCLES_TO_NS(k_cycle_get_32()) / 1000);
}
