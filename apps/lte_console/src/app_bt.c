#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <gatt/hrs.h>
#include <gatt/bas.h>
#include <gatt/cts.h>

#include <settings/settings.h>

#include <gpio.h>

#include "app_bt.h"

/* ********** DEFINES ********** */
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define MAX_DATA 74

#ifdef SW1_GPIO_INT_CONF
#define EDGE SW1_GPIO_INT_CONF
#else
/*
 * If SW0_GPIO_INT_CONF not defined used default EDGE value.
 * Change this to use a different interrupt trigger
 */
#define EDGE (GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW)
#endif

/* ********** FUNCTION DEFINITIONS ********** */
static ssize_t read_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        void *buf, u16_t len, u16_t offset);
static ssize_t write_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         const void *buf, u16_t len, u16_t offset,
                         u8_t flags);
static void vnd_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value);
static void indicate_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        u8_t err);
static ssize_t read_long_vnd(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr, void *buf,
                             u16_t len, u16_t offset);
static ssize_t write_long_vnd(struct bt_conn *conn,
                              const struct bt_gatt_attr *attr, const void *buf,
                              u16_t len, u16_t offset, u8_t flags);
static ssize_t read_signed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                           void *buf, u16_t len, u16_t offset);
static ssize_t write_signed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            const void *buf, u16_t len, u16_t offset,
                            u8_t flags);
static void connected(struct bt_conn *conn, u8_t err);
static void disconnected(struct bt_conn *conn, u8_t reason);
static void bt_ready(int err);
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey);
static void auth_cancel(struct bt_conn *conn);
static void advertButtonInit();
void advertButtonPressed(struct device *gpiob, struct gpio_callback *cb,
                         u32_t pins);
static void advertLedInit();
static void flashAdvertLed();

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
static struct bt_gatt_ccc_cfg vnd_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t simulate_vnd;
static u8_t indicating;
static struct bt_gatt_indicate_params ind_params;
static int signed_value;

static u8_t vnd_long_value[] = {
    'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '1',
    'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '2',
    'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '3',
    'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '4',
    'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '5',
    'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '6',
    '.', ' '};

static const struct bt_uuid_128 vnd_long_uuid = BT_UUID_INIT_128(
    0xf3, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
    0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static struct bt_gatt_cep vnd_long_cep = {
    .properties = BT_GATT_CEP_RELIABLE_WRITE,
};

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

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

static struct bt_conn_auth_cb auth_cb_display = {
    .passkey_display = auth_passkey_display,
    .passkey_entry = NULL,
    .cancel = auth_cancel,
};

static struct device *advertButton_dev;
static struct device *advertLed_dev;
static struct gpio_callback gpio_cb;
static bool bAdvertising = 0;
static bool bConnected = 0;
static bool bSignalAdvertising = 0;

/* ********** FUNCTIONS ********** */
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

static void connected(struct bt_conn *conn, u8_t err)
{
    if (err)
    {
        bConnected = 0;
        printk("Connection failed (err %u)\n", err);
    }
    else
    {
        bConnected = 1;
        bAdvertising = 0;
        printk("Connected\n");
    }
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
    bConnected = 0;
    bAdvertising = 1;
    printk("Disconnected (reason %u)\n", reason);
}

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
    bt_gatt_service_register(&vnd_svc);

    if (IS_ENABLED(CONFIG_SETTINGS))
    {
        settings_load();
    }
}

static void startAdvertising()
{
    int err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
                              sd, ARRAY_SIZE(sd));
    if (err)
    {
        bAdvertising = 0;
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    bAdvertising = 1;
    printk("Advertising successfully started\n");
}

static void stopAdvertising()
{
    int err = bt_le_adv_stop();
    if (err)
    {
        printk("Advertising failed to stop (err %d)\n", err);
        return;
    }

    bAdvertising = 0;
    printk("Advertising stopped\n");
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

static void advertButtonInit()
{
    int ret;

    advertButton_dev = device_get_binding(ADVERT_BUTTON_PORT);
    if (!advertButton_dev)
    {
        printk("Cannot find %s!\n", ADVERT_BUTTON_PORT);
        return;
    }

    ret = gpio_pin_configure(advertButton_dev, ADVERT_BUTTON,
                             GPIO_DIR_IN | GPIO_INT | GPIO_PUD_PULL_UP | EDGE);
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ADVERT_BUTTON);
    }

    gpio_init_callback(&gpio_cb, advertButtonPressed, BIT(ADVERT_BUTTON));
    ret = gpio_add_callback(advertButton_dev, &gpio_cb);
    if (ret)
    {
        printk("Error adding GPIO %d callback!\n", ADVERT_BUTTON);
    }

    ret = gpio_pin_enable_callback(advertButton_dev, ADVERT_BUTTON);
    if (ret)
    {
        printk("Error enabling GPIO %d callback!\n", ADVERT_BUTTON);
    }
}

void advertButtonPressed(struct device *gpiob, struct gpio_callback *cb,
                         u32_t pins)
{
    // Set flag to BT app thread can start advertising
    bSignalAdvertising = 1;
}

static void advertLedInit()
{
    int ret;

    advertLed_dev = device_get_binding(ADVERT_LED_PORT);
    if (!advertLed_dev)
    {
        printk("Cannot find %s!\n", ADVERT_LED_PORT);
        return;
    }

    ret = gpio_pin_configure(advertLed_dev, ADVERT_LED, (GPIO_DIR_OUT));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ADVERT_LED);
    }

    ret = gpio_pin_write(advertLed_dev, ADVERT_LED, LED_OFF);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", ADVERT_LED);
    }
}

static void flashAdvertLed()
{
    int ret;
    ret = gpio_pin_write(advertLed_dev, ADVERT_LED, LED_ON);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", ADVERT_LED);
    }
    k_sleep(25);
    ret = gpio_pin_write(advertLed_dev, ADVERT_LED, LED_OFF);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", ADVERT_LED);
    }
}

void app_bt_thread()
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

    advertLedInit();
    advertButtonInit();

    while (1)
    {
        k_sleep(MSEC_PER_SEC);

        if (bSignalAdvertising)
        {
            bSignalAdvertising = 0;
            if (!bAdvertising && !bConnected)
            {
                startAdvertising();
            }
            else if (bAdvertising)
            {
                stopAdvertising();
            }
        }

        if (bAdvertising)
        {
            flashAdvertLed();
        }

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

// BT thread is currently disabled. Uncomment below to add BT thread.
// K_THREAD_DEFINE(app_bt_id, 512, app_bt_thread, NULL, NULL, NULL,
//                 7, 0, K_NO_WAIT);