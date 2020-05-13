#include <zephyr.h>
#include <misc/printk.h>
#include <power.h>
#include <device.h>

#include "led.h"

#ifdef CONFIG_SYS_POWER_SLEEP_STATES
static void shutdown_console_uart(void)
{
	struct device *uart_dev;
	uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	int rc = device_set_power_state(uart_dev, DEVICE_PM_OFF_STATE, NULL,
					NULL);
	if (rc) {
		printk("Error disabling console UART peripheral (%d)\n", rc);
	}
}

static void startup_console_uart(void)
{
	struct device *uart_dev;
	uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	int rc = device_set_power_state(uart_dev, DEVICE_PM_ACTIVE_STATE, NULL,
					NULL);
	if (rc) {
		printk("Error enabling console UART peripheral (%d)\n", rc);
	}
}
#endif

/*
 * Application defined function for doing any target specific operations
 * for low power entry.
 */
void sys_pm_notify_power_state_entry(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_SYS_POWER_SLEEP_STATES
	case SYS_POWER_STATE_SLEEP_1:
		shutdown_console_uart();
		led_turn_on_isr(GREEN_LED2);
		k_cpu_idle();
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
#ifdef CONFIG_SYS_POWER_SLEEP_STATES
	case SYS_POWER_STATE_SLEEP_1:
		startup_console_uart();
		led_turn_off_isr(GREEN_LED2);
		break;
#endif
	default:
		printk("Unsupported power state %u", state);
		break;
	}
}
