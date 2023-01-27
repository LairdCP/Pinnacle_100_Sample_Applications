/*
 * Copyright (c) 2020-2023 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lte_console, LOG_LEVEL_DBG);

#include <stdlib.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/zephyr.h>
#ifdef CONFIG_MODEM_HL7800
#include <zephyr/drivers/modem/hl7800.h>
#endif

#include "led.h"
#include "version.h"

/**************************************************************************************************/
/* Local Constant, Macro and Type Definitions                                                     */
/**************************************************************************************************/

#define HEARTBEAT_INTERVAL K_SECONDS(2)
#define APN_MSG		   "APN: [%s]"

/**************************************************************************************************/
/* Local Data Definitions                                                                         */
/**************************************************************************************************/
static struct mdm_hl7800_apn *lte_apn_config;
static struct mdm_hl7800_callback_agent hl7800_evt_agent;

static struct lcz_led_blink_pattern heartbeat_pattern = {
	.on_time = 30, .off_time = 75, .repeat_count = 2};

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/

#ifdef CONFIG_MODEM_HL7800

static int shell_hl_apn_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int rc = 0;

	size_t val_len;

	if (argc == 2) {
		/* set the value */
		val_len = strlen(argv[1]);
		if (val_len > MDM_HL7800_APN_MAX_SIZE) {
			rc = -EINVAL;
			shell_error(shell, "APN too long [%d]", val_len);
			goto done;
		}

		rc = mdm_hl7800_update_apn(argv[1]);
		if (rc >= 0) {
			shell_print(shell, APN_MSG, argv[1]);
		} else {
			shell_error(shell, "Could not set APN [%d]", rc);
		}
	} else if (argc == 1) {
		/* read the value */
		shell_print(shell, APN_MSG, lte_apn_config->value);
	} else {
		shell_error(shell, "Invalid param");
		rc = -EINVAL;
	}
done:
	return rc;
}

static int shell_hl_iccid_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int rc = 0;

	shell_print(shell, "%s", mdm_hl7800_get_iccid());

	return rc;
}

static int shell_hl_imei_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int rc = 0;

	shell_print(shell, "%s", mdm_hl7800_get_imei());

	return rc;
}

static int shell_hl_pwr_off(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return mdm_hl7800_power_off();
}

static int shell_hl_reset(const struct shell *shell, size_t argc, char **argv)
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

static int shell_hl_send_at_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int rc = 0;

	if ((argc == 2) && (argv[0] != NULL) && (argv[1] != NULL)) {
		rc = mdm_hl7800_send_at_cmd(argv[1]);
		if (rc < 0) {
			shell_error(shell, "Command not accepted");
		}
	} else {
		shell_error(shell, "Wrong number of parameters");

		rc = -EINVAL;
	}

	return rc;
}

#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
static int shell_hl_fup_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int rc = 0;

	if ((argc == 2) && (argv[1] != NULL)) {
		rc = mdm_hl7800_update_fw(argv[1]);
		if (rc < 0) {
			shell_error(shell, "Command error");
		}
	} else {
		shell_error(shell, "Invalid parameter");
		rc = -EINVAL;
	}

	return rc;
}
#endif

static int shell_hl_sn_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int rc = 0;

	shell_print(shell, "%s", mdm_hl7800_get_sn());

	return rc;
}

static int shell_hl_ver_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int rc = 0;

	shell_print(shell, "%s", mdm_hl7800_get_fw_version());

	return rc;
}

static int shell_hl_site_survey_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int rc = 0;

	shell_print(shell, "survey status: %d", mdm_hl7800_perform_site_survey());

	/* Results are printed to shell by lte.site_survey_handler() */

	return rc;
}

static void hl7800_event_callback(enum mdm_hl7800_event event, void *event_data)
{
	switch (event) {
	case HL7800_EVENT_APN_UPDATE:
		lte_apn_config = (struct mdm_hl7800_apn *)event_data;
		break;
	default:
		break;
	}
}

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/

SHELL_STATIC_SUBCMD_SET_CREATE(
	hl_cmds, SHELL_CMD(apn, NULL, "HL7800 APN", shell_hl_apn_cmd),
	SHELL_CMD_ARG(cmd, NULL,
		      "Send AT command (only for advanced debug)"
		      "hl cmd <at command>",
		      shell_hl_send_at_cmd, 2, 0),
#ifdef CONFIG_MODEM_HL7800_FW_UPDATE
	SHELL_CMD_ARG(fup, NULL, "Update HL7800 firmware", shell_hl_fup_cmd, 2, 0),
#endif
	SHELL_CMD(iccid, NULL, "HL7800 SIM card ICCID", shell_hl_iccid_cmd),
	SHELL_CMD(imei, NULL, "HL7800 IMEI", shell_hl_imei_cmd),
	SHELL_CMD(pwr_off, NULL, "Power off HL7800", shell_hl_pwr_off),
	SHELL_CMD(reset, NULL, "Reset HL7800", shell_hl_reset),
	SHELL_CMD(sn, NULL, "HL7800 serial number", shell_hl_sn_cmd),
	SHELL_CMD(survey, NULL, "HL7800 site survey", shell_hl_site_survey_cmd),
	SHELL_CMD(ver, NULL, "HL7800 firmware version", shell_hl_ver_cmd),
	SHELL_CMD_ARG(wake, NULL, "Wake/Sleep HL7800", hl7800_wake, 2, 0),

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

#ifdef CONFIG_MODEM_HL7800
	hl7800_evt_agent.event_callback = hl7800_event_callback;
	mdm_hl7800_register_event_callback(&hl7800_evt_agent);
	mdm_hl7800_generate_status_events();
#endif

	while (1) {
		lcz_led_blink(GREEN_LED, &heartbeat_pattern, true);

		k_sleep(HEARTBEAT_INTERVAL);
	}
}
