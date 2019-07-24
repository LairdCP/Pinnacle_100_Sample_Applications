/* oob_ble.h - Header file for the BLE portion for out of box demo */

/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OOB_BLE_H__
#define __OOB_BLE_H__

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <misc/byteorder.h>

/* Callback function for passing sensor data */
typedef void (*sensor_updated_function_t)(u8_t sensor, s32_t reading);

struct remote_ble_sensor {
	/* State of app, see BT_DEMO_APP_STATE_XXX */
	uint8_t app_state;
	/* Handle of ESS service, used when searching for chars */
	uint8_t ess_service_handle;
	/* Temperature gatt subscribe parameters, see gatt.h for contents */
	struct bt_gatt_subscribe_params temperature_subscribe_params;
	/* Pressure gatt subscribe parameters, see gatt.h for contents */
	struct bt_gatt_subscribe_params pressure_subscribe_params;
	/* Humidity gatt subscribe parameters, see gatt.h for contents */
	struct bt_gatt_subscribe_params humidity_subscribe_params;
};

/* Function for starting BLE scan */
void bt_scan(void);

/* Function for initialising the BLE portion of the OOB demo */
void oob_ble_initialise(void);

/* This callback is triggered when BLE stack is initialised */
void bt_ready(int err);

/* This callback is triggered after recieving BLE adverts */
void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
		  struct net_buf_simple *ad);

/* This callback is triggered when notifications from remote device are received */
u8_t notify_func_callback(struct bt_conn *conn,
			  struct bt_gatt_subscribe_params *params,
			  const void *data, u16_t length);

/* This callback is triggered when remote services are discovered */
u8_t service_discover_func(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr,
			   struct bt_gatt_discover_params *params);

/* This callback is triggered when remote characteristics are discovered */
u8_t char_discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			struct bt_gatt_discover_params *params);

/* This callback is triggered when remote descriptors are discovered */
u8_t desc_discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			struct bt_gatt_discover_params *params);

/* This callback is triggered when a BLE disconnection occurs */
void disconnected(struct bt_conn *conn, u8_t reason);

/* This callback is triggered when a BLE connection occurs */
void connected(struct bt_conn *conn, u8_t err);

/* This function is used to discover services in remote device */
u8_t find_service(struct bt_conn *conn, struct bt_uuid_16 n_uuid);

/* This function is used to discover characteristics in remote device */
u8_t find_char(struct bt_conn *conn, struct bt_uuid_16 n_uuid);

/* This function is used to discover descriptors in remote device */
u8_t find_desc(struct bt_conn *conn, struct bt_uuid_16 uuid,
	       u16_t start_handle);

/* Function for setting the sensor read callback function */
void oob_ble_set_callback(sensor_updated_function_t func);

#endif /* __OOB_BLE_H__ */
