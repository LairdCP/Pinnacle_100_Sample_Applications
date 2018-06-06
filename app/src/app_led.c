#include "app_led.h"

#include <gpio.h>

static int heartbeat_led_state = LED_OFF;
static struct device *gpio_dev;

void led_init(void)
{
    int ret;

    gpio_dev = device_get_binding(LED0_GPIO_PORT);
    if (!gpio_dev)
    {
        printk("Cannot find %s!\n", LED0_GPIO_PORT);
        return;
    }

    ret = gpio_pin_configure(gpio_dev, HEARTBEAT_LED, (GPIO_DIR_OUT));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", HEARTBEAT_LED);
    }

    ret = gpio_pin_write(gpio_dev, HEARTBEAT_LED, heartbeat_led_state);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", heartbeat_led_state);
    }
}

void ledHeartBeat(void)
{
    int ret;
    /* flash LED twice quickly */
    for (u8_t i = 0; i < 2; i++)
    {
        ret = gpio_pin_write(gpio_dev, HEARTBEAT_LED, heartbeat_led_state);
        if (ret)
        {
            printk("Error setting GPIO %d!\n", HEARTBEAT_LED);
        }
        heartbeat_led_state = ~heartbeat_led_state;
        k_sleep(25);
        ret = gpio_pin_write(gpio_dev, HEARTBEAT_LED, heartbeat_led_state);
        if (ret)
        {
            printk("Error setting GPIO %d!\n", HEARTBEAT_LED);
        }
        heartbeat_led_state = ~heartbeat_led_state;
        k_sleep(50);
    }
}