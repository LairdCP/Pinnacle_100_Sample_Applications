/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr.h>
#include <power/power.h>
#include <string.h>
#include <soc.h>
#include <device.h>

#include "config.h"
#include "led.h"
#include "lte.h"
#include "http_get.h"

#define RESETREAS_REG (volatile u32_t *)0x40000400

#ifdef CONFIG_MODEM_HL7800
K_SEM_DEFINE(lte_event_sem, 0, 1);

static enum lte_event lte_status = LTE_EVT_DISCONNECTED;

static void lteEvent(enum lte_event event)
{
	lte_status = event;
	k_sem_give(&lte_event_sem);
}
#else
#include <gpio.h>
#include <uart.h>
#define GPIO0_DEV = "GPIO_0"
#define MDM_RESET_DEV "GPIO_1"
#define MDM_RESET_PIN 15
#define MDM_TX_PIN 14
#define MDM_RTS_PIN 13

static void shutdown_uart1(void)
{
#ifdef CONFIG_UART_1_NRF_UARTE
	struct device *uart_dev;
	uart_dev = device_get_binding("UART_1");

	int rc = device_set_power_state(uart_dev, DEVICE_PM_OFF_STATE, NULL,
					NULL);
	if (rc) {
		printf("Error disabling UART1 peripheral (%d)\n", rc);
	}

	struct device *gpio = device_get_binding(GPIO0_DEV);
	/* RTS */
	rc = gpio_pin_configure(gpio, MDM_TX_PIN, (GPIO_DIR_IN));
	if (rc) {
		printf("Error configuring GPIO\n");
	}
	/* TX */
	rc = gpio_pin_configure(gpio, MDM_TX_PIN, (GPIO_DIR_IN));
	if (rc) {
		printf("Error configuring GPIO\n");
	}
#endif
}
#endif /* CONFIG_MODEM_HL7800 */

/* Application main Thread */
void main(void)
{
	printf("\n\n*** Power Management Demo on %s ***\n", CONFIG_BOARD);

	/* Read the reset reason register */
	u32_t resetReas = *RESETREAS_REG;
	printf("Reset reason: 0x%08X\n", resetReas);

	led_init();

#ifdef CONFIG_MODEM_HL7800
	/* init LTE */
	lteRegisterEventCallback(lteEvent);
	int rc = lteInit();
	if (rc < 0) {
		printf("LTE init (%d)\n", rc);
		goto exit;
	}

	if (!lteIsReady()) {
		while (lte_status != LTE_EVT_READY) {
			/* Wait for LTE read evt */
			k_sem_reset(&lte_event_sem);
			printf("Waiting for LTE to be ready...\n");
			k_sem_take(&lte_event_sem, K_FOREVER);
		}
	}
	printf("LTE ready!\n");
#else
	shutdown_uart1();
	struct device *reset_gpio = device_get_binding(MDM_RESET_DEV);
	int ret = gpio_pin_configure(reset_gpio, MDM_RESET_PIN, GPIO_DIR_OUT);
	if (ret) {
		printf("Error configuring GPIO\n");
	}
	/* hold modem in reset */
	ret = gpio_pin_write(reset_gpio, MDM_RESET_PIN, 0);
	if (ret) {
		printf("Error setting GPIO state\n");
	}

#endif /* CONFIG_MODEM_HL7800 */

	while (1) {
#ifdef CONFIG_MODEM_HL7800
		http_get_execute();
		printf("App sleeping for %d seconds\n", SLEEP_TIME_SECONDS);
		k_sleep(K_SECONDS(SLEEP_TIME_SECONDS));
#endif /* CONFIG_MODEM_HL7800 */
#ifndef CONFIG_MODEM_HL7800
		printf("App busy waiting for %d seconds\n",
		       BUSY_WAIT_TIME_SECONDS);
		k_busy_wait(BUSY_WAIT_TIME);
		printf("App sleeping for %d seconds\n", SLEEP_TIME_SECONDS);
		k_sleep(K_SECONDS(SLEEP_TIME_SECONDS));
#endif
	}
#ifdef CONFIG_MODEM_HL7800
exit:
#endif /* CONFIG_MODEM_HL7800 */
	printf("Exiting main()\n");
	return;
}