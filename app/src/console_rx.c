#include <string.h>
#include <zephyr.h>
#include <misc/printk.h>
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
        default:
            printk("Unknown cmd \"%s\"\n#", cmdStr);
            break;
        }
    }
    else
    {
        char sendCmd[len];
        memset(sendCmd, 0, sizeof(sendCmd));
        switch (cmdStr[0])
        {
        case CMD_S:
            // strip s
            strncpy(sendCmd, cmdStr + 1, len - 1);
            // add cmd ending
            strcat(sendCmd, "\r");
            sendLteCmd(sendCmd);
            break;
        default:
            printk("Unknown cmd \"%s\"\n#", cmdStr);
            break;
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