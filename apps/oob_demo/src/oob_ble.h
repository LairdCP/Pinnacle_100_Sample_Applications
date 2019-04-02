
/* oob_ble.c - Header file for the BLE portion for out of box demo */

/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <misc/byteorder.h>

void oob_ble_initialise(void);

void bt_scan(void);

void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
		  struct net_buf_simple *ad);
void disconnected(struct bt_conn *conn, u8_t reason);

void connected(struct bt_conn *conn, u8_t err);

