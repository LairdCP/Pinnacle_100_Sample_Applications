#include "app_led.h"

#include <gpio.h>

static struct device *gpio_dev;

void led_init(void)
{
    int ret;

    gpio_dev = device_get_binding(HEARTBEAT_LED_PORT);
    if (!gpio_dev)
    {
        printk("Cannot find %s!\n", HEARTBEAT_LED_PORT);
        return;
    }

    ret = gpio_pin_configure(gpio_dev, HEARTBEAT_LED, (GPIO_DIR_OUT));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", HEARTBEAT_LED);
    }

    ret = gpio_pin_write(gpio_dev, HEARTBEAT_LED, LED_OFF);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", HEARTBEAT_LED);
    }

    ret = gpio_pin_configure(gpio_dev, LED1, (GPIO_DIR_OUT));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", LED1);
    }

    ret = gpio_pin_write(gpio_dev, LED1, LED_OFF);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", LED1);
    }

    ret = gpio_pin_configure(gpio_dev, LED2, (GPIO_DIR_OUT));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", LED2);
    }

    ret = gpio_pin_write(gpio_dev, LED2, LED_OFF);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", LED2);
    }

    ret = gpio_pin_configure(gpio_dev, LED3, (GPIO_DIR_OUT));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", LED3);
    }

    ret = gpio_pin_write(gpio_dev, LED3, LED_OFF);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", LED3);
    }
}

void ledHeartBeat(void)
{
    int ret;
    /* flash LED twice quickly */
    for (u8_t i = 0; i < 2; i++)
    {
        ret = gpio_pin_write(gpio_dev, HEARTBEAT_LED, LED_ON);
        if (ret)
        {
            printk("Error setting GPIO %d!\n", HEARTBEAT_LED);
        }
        k_sleep(25);
        ret = gpio_pin_write(gpio_dev, HEARTBEAT_LED, LED_OFF);
        if (ret)
        {
            printk("Error setting GPIO %d!\n", HEARTBEAT_LED);
        }
        k_sleep(50);
    }
}