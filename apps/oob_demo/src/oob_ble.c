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

void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
		  struct net_buf_simple *ad)
{
	static const char devname_expected[] = BT_REMOTE_DEVICE_NAME_STR;
	char devname_found[sizeof(devname_expected) - 1];
	uint8_t ad_element_id;
	uint8_t ad_element_len;
	uint8_t ad_element_indx = 0;
	uint8_t err;

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
				}
				break;
			}
		}
		/* Reaching here means we still didn't find device, continue searching */
		ad_element_indx += ad_element_len;
	}
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
