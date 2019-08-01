#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(app_led);

#include <gpio.h>

#include "app_led.h"

#define LED_LOG_ERR(...) LOG_ERR(__VA_ARGS__)

static struct device *led1_dev;
static struct device *led2_dev;
static struct device *led3_dev;
static struct device *led4_dev;

void led_init(void)
{
	int ret;

	led1_dev = device_get_binding(LED1_DEV);
	if (!led1_dev) {
		LED_LOG_ERR("Cannot find %s!", STRINGIFY(LED1_DEV));
		return;
	}
	led2_dev = device_get_binding(LED2_DEV);
	if (!led2_dev) {
		LED_LOG_ERR("Cannot find %s!", STRINGIFY(LED2_DEV));
		return;
	}
	led3_dev = device_get_binding(LED3_DEV);
	if (!led3_dev) {
		LED_LOG_ERR("Cannot find %s!", STRINGIFY(LED3_DEV));
		return;
	}
	led4_dev = device_get_binding(LED4_DEV);
	if (!led4_dev) {
		LED_LOG_ERR("Cannot find %s!", STRINGIFY(LED4_DEV));
		return;
	}

	/* LED1 */
	ret = gpio_pin_configure(led1_dev, LED1, GPIO_DIR_OUT);
	if (ret) {
		LED_LOG_ERR("Error configuring GPIO %s!", STRINGIFY(LED1));
	}
	ret = gpio_pin_write(led1_dev, LED1, LED_OFF);
	if (ret) {
		LED_LOG_ERR("Error setting GPIO %s!", STRINGIFY(LED1));
	}

	/* LED2 */
	ret = gpio_pin_configure(led2_dev, LED2, GPIO_DIR_OUT);
	if (ret) {
		LED_LOG_ERR("Error configuring GPIO %s!", STRINGIFY(LED2));
	}
	ret = gpio_pin_write(led2_dev, LED2, LED_OFF);
	if (ret) {
		LED_LOG_ERR("Error setting GPIO %s!", STRINGIFY(LED2));
	}

	/* LED3 */
	ret = gpio_pin_configure(led3_dev, LED3, GPIO_DIR_OUT);
	if (ret) {
		LED_LOG_ERR("Error configuring GPIO %s!", STRINGIFY(LED3));
	}
	ret = gpio_pin_write(led3_dev, LED3, LED_OFF);
	if (ret) {
		LED_LOG_ERR("Error setting GPIO %s!", STRINGIFY(LED3));
	}

	/* LED4 */
	ret = gpio_pin_configure(led4_dev, LED4, GPIO_DIR_OUT);
	if (ret) {
		LED_LOG_ERR("Error configuring GPIO %s!", STRINGIFY(LED4));
	}
	ret = gpio_pin_write(led4_dev, LED4, LED_OFF);
	if (ret) {
		LED_LOG_ERR("Error setting GPIO %s!", STRINGIFY(LED4));
	}
}

void ledHeartBeat(void)
{
	int ret;
	/* flash LED twice quickly */
	for (u8_t i = 0; i < 2; i++) {
		ret = gpio_pin_write(led2_dev, HEARTBEAT_LED, LED_ON);
		if (ret) {
			LED_LOG_ERR("Error setting GPIO %s!",
				    STRINGIFY(HEARTBEAT_LED));
		}
		k_sleep(HEARTBEAT_ON_TIME);
		ret = gpio_pin_write(led2_dev, HEARTBEAT_LED, LED_OFF);
		if (ret) {
			LED_LOG_ERR("Error setting GPIO %s!",
				    STRINGIFY(HEARTBEAT_LED));
		}
		k_sleep(HEARTBEAT_OFF_TIME);
	}
}