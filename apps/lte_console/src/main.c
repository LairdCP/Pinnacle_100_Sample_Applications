/* main.c - Application main entry point */

#include <zephyr.h>
#include <misc/printk.h>
#include <version.h>
#include <gpio.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <drivers/modem/hl7800.h>

#include "app_led.h"

#define HEARTBEAT_INTERVAL K_SECONDS(2)

static int hl7800_pwr_off(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    mdm_hl7800_power_off();
    return 0;
}

static int hl7800_reset(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    mdm_hl7800_reset();

    return 0;
}

static int hl7800_wake(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);

    int val = atoi(argv[1]);
    mdm_hl7800_wakeup(val);

    return 0;
}

static int hl7800_send_at_cmd(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);

    const u8_t *cmd = argv[1];
    mdm_hl7800_send_at_cmd(cmd);

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(hl_cmds,
                               SHELL_CMD(pwr_off, NULL, "Power off HL7800", hl7800_pwr_off),
                               SHELL_CMD(reset, NULL, "Reset HL7800", hl7800_reset),
                               SHELL_CMD_ARG(wake, NULL, "Wake/Sleep HL7800", hl7800_wake, 2, 0),
                               SHELL_CMD_ARG(send, NULL, "Send AT cmd to HL7800", hl7800_send_at_cmd, 2, 0),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(hl, &hl_cmds, "HL7800 commands", NULL);

void main(void)
{
    printk("Sirrus 100 LTE Console %d.%d.%d\nBranch: %s\nCommit: %s\nSystem up time %d us\n\n",
           APP_MAJOR, APP_MINOR, APP_PATCH,
           GIT_BRANCH, GIT_COMMIT_HASH, SYS_CLOCK_HW_CYCLES_TO_NS(k_cycle_get_32()) / 1000);
    led_init();

    while (1)
    {
        // TODO: LED heartbeat should use a timer instead of relying on the thread loop.
        // TODO: wait in main thread on event (semaphore) until some work needs to be done.
        ledHeartBeat();

        k_sleep(HEARTBEAT_INTERVAL);
    }
}
