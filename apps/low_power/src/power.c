#include <zephyr.h>
#include <misc/printk.h>
#include <power.h>
#include <device.h>

#include "led.h"

void shutdown_console_uart(void)
{
	struct device *uart_dev;
	uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	int rc = device_set_power_state(uart_dev, DEVICE_PM_OFF_STATE, NULL,
					NULL);
	if (rc) {
		printk("Error disabling console UART peripheral (%d)\n", rc);
	}
}

void startup_console_uart(void)
{
	struct device *uart_dev;
	uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	int rc = device_set_power_state(uart_dev, DEVICE_PM_ACTIVE_STATE, NULL,
					NULL);
	if (rc) {
		printk("Error disabling console UART peripheral (%d)\n", rc);
	}
}

/*
 * Application defined function for doing any target specific operations
 * for low power entry.
 */
void sys_pm_notify_power_state_entry(enum power_states state)
{
	switch (state) {
	case SYS_POWER_STATE_SLEEP_2:
		printk("--> Entering SYS_POWER_STATE_SLEEP_2 state.\n");
		shutdown_console_uart();
		led_turn_on_isr(GREEN_LED2);
		k_cpu_idle();
		break;

	case SYS_POWER_STATE_SLEEP_3:
		printk("--> Entering SYS_POWER_STATE_SLEEP_3 state.\n");
		shutdown_console_uart();
		led_turn_on_isr(GREEN_LED2);
		k_cpu_idle();
		break;
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP_STATES
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		printk("--> Entering SYS_POWER_STATE_DEEP_SLEEP_1 state.\n");
		/* Nothing else to do */
		break;
#endif
	default:
		printk("Unsupported power state %u", state);
		break;
	}
}

/*
 * Application defined function for doing any target specific operations
 * for low power exit.
 */
void sys_pm_notify_power_state_exit(enum power_states state)
{
	switch (state) {
	case SYS_POWER_STATE_SLEEP_2:
		startup_console_uart();
		printk("<-- Exited SYS_POWER_STATE_SLEEP_2 state.\n");
		led_turn_off_isr(GREEN_LED2);
		break;

	case SYS_POWER_STATE_SLEEP_3:
		startup_console_uart();
		printk("<-- Exited SYS_POWER_STATE_SLEEP_3 state.\n");
		led_turn_off_isr(GREEN_LED2);
		break;

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP_STATES
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		printk("<-- Exited SYS_POWER_STATE_DEEP_SLEEP_1 state.\n");
		break;
#endif

	default:
		printk("Unsupported power state %u", state);
		break;
	}
}
