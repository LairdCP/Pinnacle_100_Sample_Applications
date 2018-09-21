/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <uart.h>

#include "app_led.h"

#define BUF_MAXSIZE 256

static struct device *uart0_dev;
static u8_t rx_buf[BUF_MAXSIZE];
static char tx_buf[] = "ati\n";
#define TX_SIZE (sizeof(tx_buf) - 1)

static void msg_dump(const char *s, u8_t *data, unsigned len)
{
    unsigned i;

    printk("%s: ", s);
    for (i = 0; i < len; i++)
    {
        printk("%c ", data[i]);
    }
    printk("(%u bytes)\n", len);
}

static void uart0_isr(struct device *dev)
{
    /* Verify uart_irq_update() */
    if (!uart_irq_update(dev))
    {
        printk("retval should always be 1\n");
        return;
    }

    if (uart_irq_rx_ready(dev))
    {
        int len = uart_fifo_read(uart0_dev, rx_buf, BUF_MAXSIZE);
        msg_dump(__func__, rx_buf, len);
    }
}

static void uart0_init(void)
{
    uart0_dev = device_get_binding("UART_0");

    uart_irq_callback_set(uart0_dev, uart0_isr);
    uart_irq_rx_enable(uart0_dev);

    printk("%s() done\n", __func__);
}

static void send_AT_cmd(void)
{
    for (int i = 0; i < strlen(tx_buf); i++)
    {
        char sent_char = uart_poll_out(uart0_dev, tx_buf[i]);

        if (sent_char != tx_buf[i])
        {
            printk("expect send %c, actaul send %c\n",
                   tx_buf[i], sent_char);
            return;
        }
    }
}

void main(void)
{
    printk("Newcastle Test App\n");
    //uart0_init();
    //led_init();
    //send_AT_cmd();

    /* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
    while (1)
    {
        k_sleep(MSEC_PER_SEC);
        //printk("Heartbeat!\n");
        //ledHeartBeat();
    }
}
