/* oob_ble.c - BLE portion for out of box demo */

/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <bluetooth/uuid.h>

#include "oob_ble.h"

#define BT_REMOTE_DEVICE_NAME_STR                       "BL654 SENSOR"

#define BT_AD_ELEMENT_SHORTEND_LOCAL_NAME_ID            8
#define BT_AD_ELEMENT_COMPLETE_LOCAL_NAME_ID            9
#define BT_AD_SET_DATA_SIZE_MAX                         31

struct bt_conn *default_conn;

struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
		  struct net_buf_simple *ad)
{
	static const char devname_expected[] = BT_REMOTE_DEVICE_NAME_STR;
	char devname_found[sizeof(devname_expected) - 1];
	uint8_t ad_element_id;
	uint8_t ad_element_len;
	uint8_t ad_element_indx = 0;
	uint8_t err;

	/* Leave this function if already connected */
	if (default_conn) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_LE_ADV_IND && type != BT_LE_ADV_DIRECT_IND) {
		return;
	}

	/* Get device name from adverts */
	while (ad_element_indx < BT_AD_SET_DATA_SIZE_MAX) {
		ad_element_len = ad->data[ad_element_indx];
		ad_element_id = ad->data[++ad_element_indx];
		if ((ad_element_id == BT_AD_ELEMENT_SHORTEND_LOCAL_NAME_ID) || (ad_element_id == BT_AD_ELEMENT_COMPLETE_LOCAL_NAME_ID)) {
			/* Make sure that we are not exceeding advert length */
			if ((ad_element_indx + ad_element_len) <= BT_AD_SET_DATA_SIZE_MAX) {
				/* Copy device name found */
				memcpy(devname_found, &ad->data[ad_element_indx + 1], (ad_element_len - 1));

				/* Check if this is the device we are looking for */
				if (memcmp(devname_found, devname_expected, (sizeof(devname_expected) - 1)) == 0) {

					printk("Found BL654 Sensor\n");
					err = bt_le_scan_stop();
					if (err) {
						printk("Stop LE scan failed (err %d)\n", err);
					}

					/* Connect to device */
					default_conn = bt_conn_create_le(addr, BT_LE_CONN_PARAM_DEFAULT);
					if (default_conn != NULL) {
						printk("Attempting to connect to remote BLE device\n");
					} else {
						printk("Failed to connect to remote BLE device\n");
					}
				}
				break;
			}
		}
		/* Reaching here means we still didn't find device, continue searching */
		ad_element_indx += ad_element_len;
	}

}


void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		return;
	} else {
		printk("Connected: %s\n", addr);
	}
}

void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	/* Restart scanning */
	bt_scan();
}


void bt_scan(void)
{
	uint8_t err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (err) {
		printk("BLE Scan failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning for remote BLE devices started\n");
}

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);

	/* Start scanning for Bluetooth devices */
	bt_scan();
}

void oob_ble_initialise(void)
{
	int err;

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
}
