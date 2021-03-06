/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(lte_console, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <stdlib.h>
#include <version.h>
#include <drivers/gpio.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#ifdef CONFIG_MODEM_HL7800
#include <drivers/modem/hl7800.h>
#endif
#include "app_led.h"

#if CONFIG_MCUMGR
#include "mcumgr_wrapper.h"
#endif

#define HEARTBEAT_INTERVAL K_SECONDS(2)

#ifdef CONFIG_MODEM_HL7800

static int hl7800_pwr_off(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return mdm_hl7800_power_off();
}

static int hl7800_reset(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return mdm_hl7800_reset();
}

static int hl7800_wake(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int val = strtol(argv[1], NULL, 10);
	mdm_hl7800_wakeup(val);

	return 0;
}

static int hl7800_send_at_cmd(const struct shell *shell, size_t argc,
			      char **argv)
{
	ARG_UNUSED(argc);

	const uint8_t *cmd = argv[1];

	return mdm_hl7800_send_at_cmd(cmd);
}

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
static int hl7800_update(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	return mdm_hl7800_update_fw(argv[1]);
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(
	hl_cmds, SHELL_CMD(pwr_off, NULL, "Power off HL7800", hl7800_pwr_off),
	SHELL_CMD(reset, NULL, "Reset HL7800", hl7800_reset),
	SHELL_CMD_ARG(wake, NULL, "Wake/Sleep HL7800", hl7800_wake, 2, 0),
	SHELL_CMD_ARG(send, NULL, "Send AT cmd to HL7800", hl7800_send_at_cmd,
		      2, 0),
#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
	SHELL_CMD_ARG(update, NULL, "Update HL7800 firmware", hl7800_update, 2,
		      0),
#endif
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(hl, &hl_cmds, "HL7800 commands", NULL);
#endif /* CONFIG_MODEM_HL7800 */

void main(void)
{
	LOG_INF("Pinnacle 100 LTE Console %d.%d.%d\r\n"
		"Branch: %s\r\n"
		"Commit: %s\r\n",
		APP_MAJOR, APP_MINOR, APP_PATCH, GIT_BRANCH, GIT_COMMIT_HASH);
	led_init();

#if CONFIG_MCUMGR
	mcumgr_wrapper_register_subsystems();
#endif

	while (1) {
		// TODO: LED heartbeat should use a timer instead of relying on the thread loop.
		// TODO: wait in main thread on event (semaphore) until some work needs to be done.
		ledHeartBeat();

		k_sleep(HEARTBEAT_INTERVAL);
	}
}

