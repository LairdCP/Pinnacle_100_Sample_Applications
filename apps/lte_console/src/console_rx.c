#include <string.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <misc/reboot.h>
#include <console.h>

#include "console_rx.h"
#include "hl7800.h"

static void process_cmd(char *cmdStr);
static void parse_cmd(char *cmdStr);
static void console_rx_thread();

static void process_cmd(char *cmdStr)
{
    int len = strlen(cmdStr);
    if (len == 0)
    {
        printk(CMD_RESP_OK);
    }
    else
    {
        parse_cmd(cmdStr);
    }
}

static void parse_cmd(char *cmdStr)
{
    int len = strlen(cmdStr);
    if (len == 1)
    {
        switch (cmdStr[0])
        {
        case CMD_R:
            toggleLteReset();
            break;
        case CMD_W:
            toggleLteWake();
            break;
        case CMD_B:
            sys_reboot(GPREGRET_ENTER_BOOTLOADER);
            break;
        default:
            printk("Unknown cmd \"%s\"\n#", cmdStr);
            break;
        }
    }
    else
    {
        if (cmdStr[0] == 'a' && cmdStr[1] == 't')
        {
            char sendCmd[len + 1];
            memset(sendCmd, 0, sizeof(sendCmd));
            // copy bytes
            strncpy(sendCmd, cmdStr, len);
            // add cmd ending
            strcat(sendCmd, "\r");
            sendLteCmd(sendCmd);
        }
        else
        {
            printk("Unknown cmd \"%s\"\n#", cmdStr);
        }
    }
}

static void console_rx_thread()
{
    console_getline_init();

    while (1)
    {
        char *s = console_getline();
        process_cmd(s);
    }
}

K_THREAD_DEFINE(console_rx, 512, console_rx_thread, NULL, NULL, NULL,
                7, 0, K_NO_WAIT);