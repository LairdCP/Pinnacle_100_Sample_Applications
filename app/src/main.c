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

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <gatt/hrs.h>
#include <gatt/dis.h>
#include <gatt/bas.h>
#include <gatt/cts.h>

#include <uart.h>
#include <gpio.h>
#include <board.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define BUF_MAXSIZE 256

/* Custom Service Variables */
static struct bt_uuid_128 vnd_uuid = BT_UUID_INIT_128(
    0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
    0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static struct bt_uuid_128 vnd_enc_uuid = BT_UUID_INIT_128(
    0xf1, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
    0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static struct bt_uuid_128 vnd_auth_uuid = BT_UUID_INIT_128(
    0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
    0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static u8_t vnd_value[] = {'V', 'e', 'n', 'd', 'o', 'r'};

static struct device *uart0_dev;
static u8_t rx_buf[BUF_MAXSIZE];
static char tx_buf[] = "ati\n";
#define TX_SIZE (sizeof(tx_buf) - 1)
static struct device *gpio_dev;
static int led1_state = 0;

static ssize_t read_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        void *buf, u16_t len, u16_t offset)
{
    const char *value = attr->user_data;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                             strlen(value));
}

static ssize_t write_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         const void *buf, u16_t len, u16_t offset,
                         u8_t flags)
{
    u8_t *value = attr->user_data;

    if (offset + len > sizeof(vnd_value))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);

    return len;
}

static struct bt_gatt_ccc_cfg vnd_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t simulate_vnd;
static u8_t indicating;
static struct bt_gatt_indicate_params ind_params;

static void vnd_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
    simulate_vnd = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}

static void indicate_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        u8_t err)
{
    printk("Indication %s\n", err != 0 ? "fail" : "success");
    indicating = 0;
}

#define MAX_DATA 74
static u8_t vnd_long_value[] = {
    'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '1',
    'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '2',
    'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '3',
    'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '4',
    'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '5',
    'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '6',
    '.', ' '};

static ssize_t read_long_vnd(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr, void *buf,
                             u16_t len, u16_t offset)
{
    const char *value = attr->user_data;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                             sizeof(vnd_long_value));
}

static ssize_t write_long_vnd(struct bt_conn *conn,
                              const struct bt_gatt_attr *attr, const void *buf,
                              u16_t len, u16_t offset, u8_t flags)
{
    u8_t *value = attr->user_data;

    if (flags & BT_GATT_WRITE_FLAG_PREPARE)
    {
        return 0;
    }

    if (offset + len > sizeof(vnd_long_value))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);

    return len;
}

static const struct bt_uuid_128 vnd_long_uuid = BT_UUID_INIT_128(
    0xf3, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
    0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static struct bt_gatt_cep vnd_long_cep = {
    .properties = BT_GATT_CEP_RELIABLE_WRITE,
};

static int signed_value;

static ssize_t read_signed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                           void *buf, u16_t len, u16_t offset)
{
    const char *value = attr->user_data;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                             sizeof(signed_value));
}

static ssize_t write_signed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            const void *buf, u16_t len, u16_t offset,
                            u8_t flags)
{
    u8_t *value = attr->user_data;

    if (offset + len > sizeof(signed_value))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);

    return len;
}

static const struct bt_uuid_128 vnd_signed_uuid = BT_UUID_INIT_128(
    0xf3, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x13,
    0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x13);

/* Vendor Primary Service Declaration */
static struct bt_gatt_attr vnd_attrs[] = {
    /* Vendor Primary Service Declaration */
    BT_GATT_PRIMARY_SERVICE(&vnd_uuid),
    BT_GATT_CHARACTERISTIC(&vnd_enc_uuid.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
                               BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_READ_ENCRYPT |
                               BT_GATT_PERM_WRITE_ENCRYPT,
                           read_vnd, write_vnd, vnd_value),
    BT_GATT_CCC(vnd_ccc_cfg, vnd_ccc_cfg_changed),
    BT_GATT_CHARACTERISTIC(&vnd_auth_uuid.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ_AUTHEN |
                               BT_GATT_PERM_WRITE_AUTHEN,
                           read_vnd, write_vnd, vnd_value),
    BT_GATT_CHARACTERISTIC(&vnd_long_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_EXT_PROP,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
                               BT_GATT_PERM_PREPARE_WRITE,
                           read_long_vnd, write_long_vnd, &vnd_long_value),
    BT_GATT_CEP(&vnd_long_cep),
    BT_GATT_CHARACTERISTIC(&vnd_signed_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_AUTH,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_signed, write_signed, &signed_value),
};

static struct bt_gatt_service vnd_svc = BT_GATT_SERVICE(vnd_attrs);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                  0x0d, 0x18, 0x0f, 0x18, 0x05, 0x18),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
                  0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
                  0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void connected(struct bt_conn *conn, u8_t err)
{
    if (err)
    {
        printk("Connection failed (err %u)\n", err);
    }
    else
    {
        printk("Connected\n");
    }
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
    printk("Disconnected (reason %u)\n", reason);
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

static void bt_ready(int err)
{
    if (err)
    {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");

    hrs_init(0x01);
    bas_init();
    cts_init();
    dis_init(CONFIG_SOC, "Manufacturer");
    bt_gatt_service_register(&vnd_svc);

    if (IS_ENABLED(CONFIG_SETTINGS))
    {
        settings_load();
    }

    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
                          sd, ARRAY_SIZE(sd));
    if (err)
    {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
    .passkey_display = auth_passkey_display,
    .passkey_entry = NULL,
    .cancel = auth_cancel,
};

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
    static int tx_data_idx;

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

static void led_init(void)
{
    int ret;

    gpio_dev = device_get_binding(LED0_GPIO_PORT);
    if (!gpio_dev)
    {
        printk("Cannot find %s!\n", LED0_GPIO_PORT);
        return;
    }

    ret = gpio_pin_configure(gpio_dev, LED0_GPIO_PIN, (GPIO_DIR_OUT));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", LED0_GPIO_PIN);
    }
}

static void toggle_led1(void)
{
    int ret = gpio_pin_write(gpio_dev, LED0_GPIO_PIN, led1_state);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", LED0_GPIO_PIN);
    }
    if (led1_state)
    {
        led1_state = 0;
    }
    else
    {
        led1_state = 1;
    }
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
    int err;

    err = bt_enable(bt_ready);
    if (err)
    {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    bt_conn_cb_register(&conn_callbacks);
    bt_conn_auth_cb_register(&auth_cb_display);

    uart0_init();
    led_init();
    send_AT_cmd();

    /* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
    while (1)
    {
        k_sleep(MSEC_PER_SEC);
        toggle_led1();

        /* Current Time Service updates only when time is changed */
        cts_notify();

        /* Heartrate measurements simulation */
        hrs_notify();

        /* Battery level simulation */
        bas_notify();

        /* Vendor indication simulation */
        if (simulate_vnd)
        {
            if (indicating)
            {
                continue;
            }

            ind_params.attr = &vnd_attrs[2];
            ind_params.func = indicate_cb;
            ind_params.data = &indicating;
            ind_params.len = sizeof(indicating);

            if (bt_gatt_indicate(NULL, &ind_params) == 0)
            {
                indicating = 1;
            }
        }
    }
}
