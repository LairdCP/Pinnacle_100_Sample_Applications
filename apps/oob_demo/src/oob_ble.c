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

#define BT_REMOTE_DEVICE_NAME_STR                       "BL654 BME280 Sensor"

#define BT_DEMO_APP_STATE_FINDING_DEVICE                0			/* Scanning for remote sensor */
#define BT_DEMO_APP_STATE_FINDING_SERVICE               1			/* Searching for ESS service */
#define BT_DEMO_APP_STATE_FINDING_TEMP_CHAR             2			/* Searching for ESS Temperature characteristic */
#define BT_DEMO_APP_STATE_FINDING_HUMIDITY_CHAR         3			/* Searching for ESS Humidity characteristic */
#define BT_DEMO_APP_STATE_FINDING_PRESSURE_CHAR         4			/* Searching for ESS Pressure characteristic */

#define BT_AD_ELEMENT_SHORTEND_LOCAL_NAME_ID            8
#define BT_AD_ELEMENT_COMPLETE_LOCAL_NAME_ID            9
#define BT_AD_SET_DATA_SIZE_MAX                         31

struct bt_conn *default_conn;

struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

struct bt_gatt_discover_params discover_params;

struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

struct remote_ble_sensor remote_ble_sensor_params;

/* This callback is triggered after recieving BLE adverts */
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


/* This callback is triggered when notifications from remote device are received */
u8_t notify_func_callback(struct bt_conn *conn,
			  struct bt_gatt_subscribe_params *params,
			  const void *data, u16_t length)
{
	if (!data) {
		printk("Unsubscribed\n");
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	/* Check if the notifications received have the temperature handle */
	if (params->value_handle == remote_ble_sensor_params.temperature_subscribe_params.value_handle) {
		/* Temperature is a 16 bit value */
		uint8_t temperature_data[2];
		memcpy(temperature_data, data, length);
		printk("ESS Temperature value = %d\n", ((temperature_data[1]<<8 & 0xFF00)) + (temperature_data[0]));
	}
	/* Check if the notifications received have the humidity handle */
	else if (params->value_handle == remote_ble_sensor_params.humidity_subscribe_params.value_handle) {
		/* Humidity is a 16 bit value */
		uint8_t humidity_data[2];
		memcpy(humidity_data, data, length);
		printk("ESS Humidity value = %d\n", ((humidity_data[1]<<8) & 0xFF00) + humidity_data[0]);
	}
	/* Check if the notifications received have the pressure handle */
	else if (params->value_handle == remote_ble_sensor_params.pressure_subscribe_params.value_handle) {
		/*Pressure is a 32 bit value */
		uint8_t pressure_data[4];
		memcpy(pressure_data, data, length);
		printk("ESS Pressure value = %d\n", (((pressure_data[3]<<24) & 0xFF000000) + ((pressure_data[2]<<16) & 0xFF0000) + ((pressure_data[1]<<8) & 0xFF00) + pressure_data[0]));
	}

	return BT_GATT_ITER_CONTINUE;
}

/* This function is used to discover descriptors in remote device */
u8_t find_desc(struct bt_conn *conn, struct bt_uuid_16 uuid, u16_t start_handle)
{
	uint8_t err;

	/* Update discover parameters before initiating discovery */
	discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
	discover_params.uuid = &uuid.uuid;
	discover_params.start_handle = start_handle;
	discover_params.func = desc_discover_func;

	/* Call zephyr library function */
	err = bt_gatt_discover(conn, &discover_params);

	return err;
}

/* This function is used to discover characteristics in remote device */
u8_t find_char(struct bt_conn *conn, struct bt_uuid_16 n_uuid)
{
	uint8_t err;

	/* Update discover parameters before initiating discovery */
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	discover_params.uuid = &uuid.uuid;
	discover_params.start_handle = remote_ble_sensor_params.ess_service_handle;
	discover_params.func = char_discover_func;

	/* Call zephyr library function */
	err = bt_gatt_discover(conn, &discover_params);

	return err;
}

/* This function is used to discover services in remote device */
u8_t find_service(struct bt_conn *conn, struct bt_uuid_16 n_uuid)
{

	uint8_t err;

	/* Update discover parameters before initiating discovery */
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.uuid = &uuid.uuid;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;
	discover_params.func = service_discover_func;

	/* Call zephyr library function */
	err = bt_gatt_discover(default_conn, &discover_params);
	return err;
}

/* This callback is triggered when remote descriptors are discovered */
u8_t desc_discover_func(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			struct bt_gatt_discover_params *params)
{
	uint8_t err;

	if (remote_ble_sensor_params.app_state == BT_DEMO_APP_STATE_FINDING_TEMP_CHAR) {
		/* Found temperature CCCD, enable notifications and move on */
		remote_ble_sensor_params.temperature_subscribe_params.notify = notify_func_callback;
		remote_ble_sensor_params.temperature_subscribe_params.value = BT_GATT_CCC_NOTIFY;
		remote_ble_sensor_params.temperature_subscribe_params.ccc_handle = attr->handle;
		err = bt_gatt_subscribe(conn, &remote_ble_sensor_params.temperature_subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			printk("Notifications enabled for temperature characteristic\n");

			/* Now go on to find humidity characteristic */
			memcpy(&uuid, BT_UUID_HUMIDITY, sizeof(uuid));
			err = find_char(conn, uuid);

			if (err) {
				printk("Discover failed (err %d)\n", err);

				/* couldn't discover char, disconnect */
				bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			} else {
				/* everything looks good, update state to looking for humidity char */
				remote_ble_sensor_params.app_state = BT_DEMO_APP_STATE_FINDING_HUMIDITY_CHAR;
			}
		}
	} else if (remote_ble_sensor_params.app_state == BT_DEMO_APP_STATE_FINDING_HUMIDITY_CHAR) {
		/* Found humidity CCCD, enable notifications and move on */
		remote_ble_sensor_params.humidity_subscribe_params.notify = notify_func_callback;
		remote_ble_sensor_params.humidity_subscribe_params.value = BT_GATT_CCC_NOTIFY;
		remote_ble_sensor_params.humidity_subscribe_params.ccc_handle = attr->handle;
		err = bt_gatt_subscribe(conn, &remote_ble_sensor_params.humidity_subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			printk("Notifications enabled for humidity characteristic\n");
			/* Now go on to find pressure characteristic */

			memcpy(&uuid, BT_UUID_PRESSURE, sizeof(uuid));
			err = find_char(conn, uuid);

			if (err) {
				printk("Discover failed (err %d)\n", err);

				/* couldn't discover char, disconnect */
				bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			} else {
				/* everything looks good, update state to looking for pressure char */
				remote_ble_sensor_params.app_state = BT_DEMO_APP_STATE_FINDING_PRESSURE_CHAR;
			}
		}
	} else if (remote_ble_sensor_params.app_state == BT_DEMO_APP_STATE_FINDING_PRESSURE_CHAR) {
		/* Found pressure CCCD, enable notifications and move on */
		remote_ble_sensor_params.pressure_subscribe_params.notify = notify_func_callback;
		remote_ble_sensor_params.pressure_subscribe_params.value = BT_GATT_CCC_NOTIFY;
		remote_ble_sensor_params.pressure_subscribe_params.ccc_handle = attr->handle;
		err = bt_gatt_subscribe(conn, &remote_ble_sensor_params.pressure_subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			printk("Notifications enabled for pressure characteristic\n");
		}
	}

	return BT_GATT_ITER_STOP;
}

/* This callback is triggered when remote characteristics are discovered */
u8_t char_discover_func(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			struct bt_gatt_discover_params *params)
{
	uint8_t err;

	if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_TEMPERATURE)) {
		printk("Found ESS Temperature characteristic\n");

		/* Update global structure with the handles of the temperature characteristic */
		remote_ble_sensor_params.temperature_subscribe_params.value_handle = attr->handle + 1;
		/* Now start searching for CCCD within temperature characteristic */
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		err = find_desc(conn, uuid, attr->handle + 2);
		if (err) {
			printk("Discover failed (err %d)\n", err);
			bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HUMIDITY)) {
		printk("Found ESS Humidity characteristic\n");

		/* Update global structure with the handles of the temperature characteristic */
		remote_ble_sensor_params.humidity_subscribe_params.value_handle = attr->handle + 1;
		/* Now start searching for CCCD within temperature characteristic */
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		err = find_desc(conn, uuid, attr->handle + 2);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_PRESSURE)) {
		printk("Found ESS Pressure characteristic\n");

		/* Update global structure with the handles of the temperature characteristic */
		remote_ble_sensor_params.pressure_subscribe_params.value_handle = attr->handle + 1;
		/* Now start searching for CCCD within temperature characteristic */
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		err = find_desc(conn, uuid, attr->handle + 2);
		if (err) {
			printk("Discover failed (err %d)\n", err);
			bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
	}

	return BT_GATT_ITER_STOP;
}

/* This callback is triggered when remote services are discovered */
u8_t service_discover_func(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr,
			   struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_ESS)) {
		/* Found ESS Service, start searching for temperature char */
		printk("Found ESS Service\n");
		/* Update service handle global variable as it will be used when searching for chars */
		remote_ble_sensor_params.ess_service_handle = attr->handle + 1;
		/* Start looking for temperature characteristic */
		memcpy(&uuid, BT_UUID_TEMPERATURE, sizeof(uuid));
		err = find_char(conn, uuid);
		if (err) {
			printk("Discover failed (err %d)\n", err);

			/* couldn't discover char, disconnect */
			bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		} else {
			/* Looking for temperature char, update state */
			remote_ble_sensor_params.app_state = BT_DEMO_APP_STATE_FINDING_TEMP_CHAR;
		}
	}

	return BT_GATT_ITER_STOP;
}

/* This callback is triggered when a BLE connection occurs */
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

	if (conn == default_conn) {
		memcpy(&uuid, BT_UUID_ESS, sizeof(uuid));

		err = find_service(conn, uuid);

		if (err) {
			printk("Discover failed(err %d)\n", err);
			return;
		}
	}

	/* Reaching here means all is good, update state */
	remote_ble_sensor_params.app_state = BT_DEMO_APP_STATE_FINDING_SERVICE;
}

/* This callback is triggered when a BLE disconnection occurs */
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

/* Function for starting BLE scan */
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

/* This callback is triggered when BLE stack is initalised */
void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);

	/* Initialize the state to 'looking for device' */
	remote_ble_sensor_params.app_state = BT_DEMO_APP_STATE_FINDING_DEVICE;

	/* Start scanning for Bluetooth devices */
	bt_scan();
}

/* Function for initialising the BLE portion of the OOB demo */
void oob_ble_initialise(void)
{
	int err;

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
}
