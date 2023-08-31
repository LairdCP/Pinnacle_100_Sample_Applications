/*
 * Copyright (c) 2020-2023 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <soc.h>
#if defined(CONFIG_LCZ_NETWORK_MONITOR)
#include <lcz_network_monitor.h>
#endif
#if defined(CONFIG_MODEM_HL7800)
#include <zephyr/drivers/modem/hl7800.h>
#endif

#include "config.h"
#include "http_get.h"

#define RESETREAS_REG (volatile uint32_t *)0x40000400
static int loop_count = 0;
static const struct device *qspi_flash = DEVICE_DT_GET(DT_NODELABEL(mx25r64));

#ifdef CONFIG_MODEM_HL7800
static bool http_keep_alive = true;
static struct lcz_nm_event_agent nm_event_agent;
static K_SEM_DEFINE(lte_event_sem, 0, 1);

static int start_and_ready_modem(void)
{
	int ret;

	printf("Starting modem...\n");
	ret = mdm_hl7800_reset();
	if (ret != 0) {
		printf("Error starting modem [%d]\n", ret);
		return ret;
	} else {
		printf("Modem started!\n");
	}

	do {
		printf("Waiting for LTE to be ready...\n");
		ret = k_sem_take(&lte_event_sem, K_SECONDS(30));
	} while (ret != 0);

	return ret;
}

#else
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#define GPIO0_DEV     "GPIO_0"
#define MDM_RESET_DEV "GPIO_1"
#define MDM_RESET_PIN 15
#define MDM_TX_PIN    14
#define MDM_RTS_PIN   13

static void shutdown_uart1(void)
{
#ifdef CONFIG_UART_1_NRF_UARTE
	const struct device *uart_dev;
	enum pm_device_state state;
	int rc;

	uart_dev = device_get_binding("UART_1");
	if (uart_dev == NULL) {
		printf("No device UART_1\n");
		return;
	}

	(void)pm_device_state_get(uart_dev, &state);
	if (state != PM_DEVICE_STATE_SUSPENDED) {
		rc = pm_device_action_run(uart_dev, PM_DEVICE_ACTION_SUSPEND);
		if (rc) {
			printf("Error disabling UART1 peripheral (%d)\n", rc);
		}
	}

	const struct device *gpio = device_get_binding(GPIO0_DEV);
	/* RTS */
	rc = gpio_pin_configure(gpio, MDM_RTS_PIN, (GPIO_INPUT));
	if (rc) {
		printf("Error configuring GPIO\n");
	}
	/* TX */
	rc = gpio_pin_configure(gpio, MDM_TX_PIN, (GPIO_INPUT));
	if (rc) {
		printf("Error configuring GPIO\n");
	}
#endif
}
#endif /* CONFIG_MODEM_HL7800 */

#if defined(CONFIG_LOG)
#if defined(CONFIG_SHELL_BACKEND_SERIAL)
#define BACKEND_NAME "shell_uart_backend"
#elif defined(CONFIG_LOG_BACKEND_UART)
#define BACKEND_NAME "log_backend_uart"
#else
#error "Unhandled backend"
#endif

static const struct log_backend *uart_backend;

/** @brief Finds the logging subsystem backend pointer for UART.
 *
 *  @returns 0 for success, < 0 if not found.
 */
static int log_find_uart_backend(void)
{
	int ret = -ENOENT;
	const struct log_backend *backend;
	uint32_t backend_idx;


	uart_backend = NULL;
	for (backend_idx = 0; (uart_backend == NULL); backend_idx++) {
		/* Get the next backend */
		backend = log_backend_get(backend_idx);
		/* If it's NULL, stop here */
		if (backend != NULL) {
			/* Is it UART backend? */
			if (!strcmp(backend->name, BACKEND_NAME)) {
				/* Found it */
				uart_backend = backend;
				ret = 0;
				break;
			}
		} else {
			break;
		}
	}
	return ret;
}

#if defined(CONFIG_SHUTDOWN_CONSOLE_UART)
/** @brief Disables the logging subsystem backend UART.
 *
 */
static void uart_console_logging_disable(void)
{
	if (uart_backend != NULL) {
		log_backend_deactivate(uart_backend);
	}
}

/** @brief Enables the logging subsystem backend UART.
 *
 */
static void uart_console_logging_enable(void)
{
	if (uart_backend != NULL) {
		log_backend_activate(uart_backend, uart_backend->cb->ctx);
	}
}
#endif
#endif

static void shutdown_console_uart(void)
{
#ifdef CONFIG_SHUTDOWN_CONSOLE_UART
	const struct device *uart_dev;
	enum pm_device_state state;
	int rc;

	uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (uart_dev == NULL) {
		printf("No console UART device!\n");
		return;
	}
	(void)pm_device_state_get(uart_dev, &state);
	if (state != PM_DEVICE_STATE_SUSPENDED) {
		rc = pm_device_action_run(uart_dev, PM_DEVICE_ACTION_SUSPEND);
		if (rc) {
			printf("Error disabling console UART peripheral (%d)\n", rc);
		}
	}
#endif
}

static void startup_console_uart(void)
{
#ifdef CONFIG_SHUTDOWN_CONSOLE_UART
	const struct device *uart_dev;
	enum pm_device_state state;
	int rc;

	uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (uart_dev == NULL) {
		printf("No console UART device!\n");
		return;
	}
	(void)pm_device_state_get(uart_dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		rc = pm_device_action_run(uart_dev, PM_DEVICE_ACTION_RESUME);
		if (rc) {
			printf("Error enabling console UART peripheral (%d)\n", rc);
		}
	}
#endif
}

#if defined(CONFIG_NETWORKING)
static void nm_event_callback(enum lcz_nm_event event)
{
	printf("Network monitor event %d\n", event);
	switch (event) {
	case LCZ_NM_EVENT_IFACE_DNS_ADDED:
		k_sem_give(&lte_event_sem);
		break;
	case LCZ_NM_EVENT_IFACE_DOWN:
		k_sem_reset(&lte_event_sem);
	default:
		break;
	}
}
#endif

/* Application main Thread */
int main(void)
{
	int ret = 0;
	int sleep_seconds = SLEEP_TIME_SECONDS;

	printf("\n\n*** Power Management Demo on %s ***\n", CONFIG_BOARD);

	/* Read the reset reason register */
	uint32_t resetReas = *RESETREAS_REG;
	printf("Reset reason: 0x%08X\n", resetReas);

#if defined(CONFIG_LOG)
	ret = log_find_uart_backend();
	if (ret != 0) {
		printf("ERROR: Could not find log backend!\n");
	}
#endif

	ret = pm_device_runtime_enable(qspi_flash);
	if (ret != 0) {
		printf("ERROR: Could not enable QSPI flash runtime PM [%d]\n", ret);
	}

#ifdef CONFIG_MODEM_HL7800
	nm_event_agent.callback = nm_event_callback;
	lcz_nm_register_event_callback(&nm_event_agent);

	ret = start_and_ready_modem();
	if (ret != 0) {
		goto exit;
	}
#else
	shutdown_uart1();
	const struct device *reset_gpio = device_get_binding(MDM_RESET_DEV);
	ret = gpio_pin_configure(reset_gpio, MDM_RESET_PIN, GPIO_OUTPUT);
	if (ret) {
		printf("Error configuring GPIO\n");
	}
	/* hold modem in reset */
	ret = gpio_pin_set_raw(reset_gpio, MDM_RESET_PIN, 0);
	if (ret) {
		printf("Error setting GPIO state\n");
	}

#endif /* CONFIG_MODEM_HL7800 */

	while (1) {
		loop_count++;
		startup_console_uart();
#if defined(CONFIG_LOG) && defined(CONFIG_SHUTDOWN_CONSOLE_UART)
		uart_console_logging_enable();
#endif
#ifdef CONFIG_MODEM_HL7800
#if !defined(CONFIG_MODEM_HL7800_SLEEP_LEVEL_SLEEP) || defined(CONFIG_MODEM_HL7800_PSM) ||         \
	defined(CONFIG_POWER_OFF_MODEM)
		http_keep_alive = false;
#else
		if (loop_count % 3 == 0) {
			http_keep_alive = !http_keep_alive;
		}
#endif
#if defined(CONFIG_POWER_OFF_MODEM)
		if (loop_count != 1) {
			ret = start_and_ready_modem();
		}
#endif
		if (ret == 0) {
			http_get_execute(http_keep_alive);
		}
#endif /* CONFIG_MODEM_HL7800 */
#ifndef CONFIG_MODEM_HL7800
		printf("App busy waiting for %d seconds\n", BUSY_WAIT_TIME_SECONDS);
		k_busy_wait(BUSY_WAIT_TIME);
#endif
#if defined(CONFIG_POWER_OFF_MODEM)
		printf("Power off modem...\n");
		ret = mdm_hl7800_power_off();
		if (ret == 0) {
			printf("Modem off\n");
		} else {
			printf("Err turning modem off [%d]\n", ret);
		}
#endif
		if (http_keep_alive) {
			sleep_seconds = SLEEP_TIME_KEEP_ALIVE_SECONDS;
			printf("Reducing sleep time to keep the socket alive\n");
		} else {
			sleep_seconds = SLEEP_TIME_SECONDS;
		}
		printf("App sleeping for %d seconds\n", sleep_seconds);
#if defined(CONFIG_LOG) && defined(CONFIG_SHUTDOWN_CONSOLE_UART)
		uart_console_logging_disable();
#endif
		shutdown_console_uart();
		k_sleep(K_SECONDS(sleep_seconds));
	}
#ifdef CONFIG_MODEM_HL7800
exit:
#endif /* CONFIG_MODEM_HL7800 */
	printf("Exiting main()\n");
	shutdown_console_uart();
	return -1;
}
