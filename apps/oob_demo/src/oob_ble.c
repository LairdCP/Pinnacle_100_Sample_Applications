/* oob_ble.c - BLE portion for out of box demo */

/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(oob_ble);

#include <zephyr.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <bluetooth/uuid.h>
#include <settings/settings.h>
#include <version.h>

#include "oob_common.h"
#include "oob_ble.h"

#define BLE_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define BLE_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define BLE_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define BLE_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

#define BT_REMOTE_DEVICE_NAME_STR "BL654 BME280 Sensor"

#define BT_DEMO_APP_STATE_FINDING_DEVICE 0 /* Scanning for remote sensor */
#define BT_DEMO_APP_STATE_FINDING_SERVICE 1 /* Searching for ESS service */
#define BT_DEMO_APP_STATE_FINDING_TEMP_CHAR                                    \
	2 /* Searching for ESS Temperature characteristic */
#define BT_DEMO_APP_STATE_FINDING_HUMIDITY_CHAR                                \
	3 /* Searching for ESS Humidity characteristic */
#define BT_DEMO_APP_STATE_FINDING_PRESSURE_CHAR                                \
	4 /* Searching for ESS Pressure characteristic */

#define BT_AD_ELEMENT_SHORTEND_LOCAL_NAME_ID 8
#define BT_AD_ELEMENT_COMPLETE_LOCAL_NAME_ID 9
#define BT_AD_SET_DATA_SIZE_MAX 31

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, 0x36, 0xa3, 0x4d, 0x40, 0xb6, 0x70,
		      0x69, 0xa6, 0xb1, 0x4e, 0x84, 0x9e, 0x60, 0x7c, 0x78,
		      0x43),
};

struct bt_conn *sensor_conn;

struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

struct bt_gatt_discover_params discover_params;

struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

struct remote_ble_sensor remote_ble_sensor_params;

sensor_updated_function_t SensorCallbackFunction = NULL;

static int zephyr_settings_fw_load(struct settings_store *cs,
				   const char *subtree);

static const struct settings_store_itf zephyr_settings_fw_itf = {
	.csi_load = zephyr_settings_fw_load,
};

static struct settings_store zephyr_settings_fw_store = {
	.cs_itf = &zephyr_settings_fw_itf
};

/* This callback is triggered after recieving BLE adverts */
void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
		  struct net_buf_simple *ad)
{
	char bt_addr[BT_ADDR_LE_STR_LEN];
	static const char devname_expected[] = BT_REMOTE_DEVICE_NAME_STR;
	char devname_found[sizeof(devname_expected) - 1];
	u8_t ad_element_id;
	u8_t ad_element_len;
	u8_t ad_element_indx = 0;
	u8_t err;

	/* Leave this function if already connected */
	if (sensor_conn) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_LE_ADV_IND && type != BT_LE_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, bt_addr, sizeof(bt_addr));

	/* Get device name from adverts */
	while (ad_element_indx < BT_AD_SET_DATA_SIZE_MAX) {
		ad_element_len = ad->data[ad_element_indx];
		ad_element_id = ad->data[++ad_element_indx];
		if ((ad_element_id == BT_AD_ELEMENT_SHORTEND_LOCAL_NAME_ID) ||
		    (ad_element_id == BT_AD_ELEMENT_COMPLETE_LOCAL_NAME_ID)) {
			/* Make sure that we are not exceeding advert length */
			if ((ad_element_indx + ad_element_len) <=
			    BT_AD_SET_DATA_SIZE_MAX) {
				/* Copy device name found */
				memcpy(devname_found,
				       &ad->data[ad_element_indx + 1],
				       (ad_element_len - 1));

				/* Check if this is the device we are looking for */
				if (memcmp(devname_found, devname_expected,
					   (sizeof(devname_expected) - 1)) ==
				    0) {
					BLE_LOG_INF("Found BL654 Sensor");
					err = bt_le_scan_stop();
					if (err) {
						BLE_LOG_ERR(
							"Stop LE scan failed (err %d)",
							err);
					}

					/* Connect to device */
					sensor_conn = bt_conn_create_le(
						addr, BT_LE_CONN_PARAM_DEFAULT);
					if (sensor_conn != NULL) {
						BLE_LOG_INF(
							"Attempting to connect to remote BLE device %s",
							log_strdup(bt_addr));
					} else {
						BLE_LOG_ERR(
							"Failed to connect to remote BLE device %s",
							log_strdup(bt_addr));
						/* restart scanning */
						bt_scan();
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
		BLE_LOG_INF("Unsubscribed");
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	/* Check if the notifications received have the temperature handle */
	if (params->value_handle ==
	    remote_ble_sensor_params.temperature_subscribe_params.value_handle) {
		/* Temperature is a 16 bit value */
		u8_t temperature_data[2];
		memcpy(temperature_data, data, length);
		BLE_LOG_INF("ESS Temperature value = %d",
			    ((temperature_data[1] << 8 & 0xFF00)) +
				    (temperature_data[0]));
		if (SensorCallbackFunction != NULL) {
			//Pass data to callback function
			SensorCallbackFunction(
				SENSOR_TYPE_TEMPERATURE,
				((temperature_data[1] << 8 & 0xFF00)) +
					(temperature_data[0]));
		}
	}
	/* Check if the notifications received have the humidity handle */
	else if (params->value_handle ==
		 remote_ble_sensor_params.humidity_subscribe_params
			 .value_handle) {
		/* Humidity is a 16 bit value */
		u8_t humidity_data[2];
		memcpy(humidity_data, data, length);
		BLE_LOG_INF("ESS Humidity value = %d",
			    ((humidity_data[1] << 8) & 0xFF00) +
				    humidity_data[0]);
		if (SensorCallbackFunction != NULL) {
			//Pass data to callback function
			SensorCallbackFunction(SENSOR_TYPE_HUMIDITY,
					       ((humidity_data[1] << 8) &
						0xFF00) +
						       humidity_data[0]);
		}
	}
	/* Check if the notifications received have the pressure handle */
	else if (params->value_handle ==
		 remote_ble_sensor_params.pressure_subscribe_params
			 .value_handle) {
		/*Pressure is a 32 bit value */
		u8_t pressure_data[4];
		memcpy(pressure_data, data, length);
		BLE_LOG_INF("ESS Pressure value = %d",
			    (((pressure_data[3] << 24) & 0xFF000000) +
			     ((pressure_data[2] << 16) & 0xFF0000) +
			     ((pressure_data[1] << 8) & 0xFF00) +
			     pressure_data[0]));
		if (SensorCallbackFunction != NULL) {
			//Pass data to callback function
			SensorCallbackFunction(
				SENSOR_TYPE_PRESSURE,
				(((pressure_data[3] << 24) & 0xFF000000) +
				 ((pressure_data[2] << 16) & 0xFF0000) +
				 ((pressure_data[1] << 8) & 0xFF00) +
				 pressure_data[0]));
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

/* This function is used to discover descriptors in remote device */
u8_t find_desc(struct bt_conn *conn, struct bt_uuid_16 uuid, u16_t start_handle)
{
	u8_t err;

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
	u8_t err;

	/* Update discover parameters before initiating discovery */
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	discover_params.uuid = &uuid.uuid;
	discover_params.start_handle =
		remote_ble_sensor_params.ess_service_handle;
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
	err = bt_gatt_discover(sensor_conn, &discover_params);
	return err;
}

/* This callback is triggered when remote descriptors are discovered */
u8_t desc_discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			struct bt_gatt_discover_params *params)
{
	u8_t err;

	if (remote_ble_sensor_params.app_state ==
	    BT_DEMO_APP_STATE_FINDING_TEMP_CHAR) {
		/* Found temperature CCCD, enable notifications and move on */
		remote_ble_sensor_params.temperature_subscribe_params.notify =
			notify_func_callback;
		remote_ble_sensor_params.temperature_subscribe_params.value =
			BT_GATT_CCC_NOTIFY;
		remote_ble_sensor_params.temperature_subscribe_params
			.ccc_handle = attr->handle;
		err = bt_gatt_subscribe(
			conn,
			&remote_ble_sensor_params.temperature_subscribe_params);
		if (err && err != -EALREADY) {
			BLE_LOG_ERR("Subscribe failed (err %d)", err);
		} else {
			BLE_LOG_INF(
				"Notifications enabled for temperature characteristic");

			/* Now go on to find humidity characteristic */
			memcpy(&uuid, BT_UUID_HUMIDITY, sizeof(uuid));
			err = find_char(conn, uuid);

			if (err) {
				BLE_LOG_ERR("Discover failed (err %d)", err);

				/* couldn't discover char, disconnect */
				bt_conn_disconnect(
					conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			} else {
				/* everything looks good, update state to looking for humidity char */
				remote_ble_sensor_params.app_state =
					BT_DEMO_APP_STATE_FINDING_HUMIDITY_CHAR;
			}
		}
	} else if (remote_ble_sensor_params.app_state ==
		   BT_DEMO_APP_STATE_FINDING_HUMIDITY_CHAR) {
		/* Found humidity CCCD, enable notifications and move on */
		remote_ble_sensor_params.humidity_subscribe_params.notify =
			notify_func_callback;
		remote_ble_sensor_params.humidity_subscribe_params.value =
			BT_GATT_CCC_NOTIFY;
		remote_ble_sensor_params.humidity_subscribe_params.ccc_handle =
			attr->handle;
		err = bt_gatt_subscribe(
			conn,
			&remote_ble_sensor_params.humidity_subscribe_params);
		if (err && err != -EALREADY) {
			BLE_LOG_ERR("Subscribe failed (err %d)", err);
		} else {
			BLE_LOG_INF(
				"Notifications enabled for humidity characteristic");
			/* Now go on to find pressure characteristic */

			memcpy(&uuid, BT_UUID_PRESSURE, sizeof(uuid));
			err = find_char(conn, uuid);

			if (err) {
				BLE_LOG_ERR("Discover failed (err %d)", err);

				/* couldn't discover char, disconnect */
				bt_conn_disconnect(
					conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			} else {
				/* everything looks good, update state to looking for pressure char */
				remote_ble_sensor_params.app_state =
					BT_DEMO_APP_STATE_FINDING_PRESSURE_CHAR;
			}
		}
	} else if (remote_ble_sensor_params.app_state ==
		   BT_DEMO_APP_STATE_FINDING_PRESSURE_CHAR) {
		/* Found pressure CCCD, enable notifications and move on */
		remote_ble_sensor_params.pressure_subscribe_params.notify =
			notify_func_callback;
		remote_ble_sensor_params.pressure_subscribe_params.value =
			BT_GATT_CCC_NOTIFY;
		remote_ble_sensor_params.pressure_subscribe_params.ccc_handle =
			attr->handle;
		err = bt_gatt_subscribe(
			conn,
			&remote_ble_sensor_params.pressure_subscribe_params);
		if (err && err != -EALREADY) {
			BLE_LOG_ERR("Subscribe failed (err %d)", err);
		} else {
			BLE_LOG_INF(
				"Notifications enabled for pressure characteristic");
		}
	}

	return BT_GATT_ITER_STOP;
}

/* This callback is triggered when remote characteristics are discovered */
u8_t char_discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			struct bt_gatt_discover_params *params)
{
	u8_t err;

	if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_TEMPERATURE)) {
		BLE_LOG_DBG("Found ESS Temperature characteristic");

		/* Update global structure with the handles of the temperature characteristic */
		remote_ble_sensor_params.temperature_subscribe_params
			.value_handle = attr->handle + 1;
		/* Now start searching for CCCD within temperature characteristic */
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		err = find_desc(conn, uuid, attr->handle + 2);
		if (err) {
			BLE_LOG_ERR("Discover failed (err %d)", err);
			bt_conn_disconnect(conn,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HUMIDITY)) {
		BLE_LOG_DBG("Found ESS Humidity characteristic");

		/* Update global structure with the handles of the temperature characteristic */
		remote_ble_sensor_params.humidity_subscribe_params.value_handle =
			attr->handle + 1;
		/* Now start searching for CCCD within temperature characteristic */
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		err = find_desc(conn, uuid, attr->handle + 2);
		if (err) {
			BLE_LOG_ERR("Discover failed (err %d)", err);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_PRESSURE)) {
		BLE_LOG_DBG("Found ESS Pressure characteristic");

		/* Update global structure with the handles of the temperature characteristic */
		remote_ble_sensor_params.pressure_subscribe_params.value_handle =
			attr->handle + 1;
		/* Now start searching for CCCD within temperature characteristic */
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		err = find_desc(conn, uuid, attr->handle + 2);
		if (err) {
			BLE_LOG_ERR("Discover failed (err %d)", err);
			bt_conn_disconnect(conn,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
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
		BLE_LOG_DBG("Discover complete");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_ESS)) {
		/* Found ESS Service, start searching for temperature char */
		BLE_LOG_DBG("Found ESS Service");
		/* Update service handle global variable as it will be used when searching for chars */
		remote_ble_sensor_params.ess_service_handle = attr->handle + 1;
		/* Start looking for temperature characteristic */
		memcpy(&uuid, BT_UUID_TEMPERATURE, sizeof(uuid));
		err = find_char(conn, uuid);
		if (err) {
			BLE_LOG_ERR("Discover failed (err %d)", err);

			/* couldn't discover char, disconnect */
			bt_conn_disconnect(conn,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		} else {
			/* Looking for temperature char, update state */
			remote_ble_sensor_params.app_state =
				BT_DEMO_APP_STATE_FINDING_TEMP_CHAR;
		}
	}

	return BT_GATT_ITER_STOP;
}

/* This callback is triggered when a BLE connection occurs */
void connected(struct bt_conn *conn, u8_t err)
{
	int rc;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		goto fail;
	}

	if (conn == sensor_conn) {
		BLE_LOG_INF("Connected sensor: %s", log_strdup(addr));

		memcpy(&uuid, BT_UUID_ESS, sizeof(uuid));

		err = find_service(conn, uuid);

		if (err) {
			BLE_LOG_ERR("Discover failed(err %d)", err);
			goto fail;
		}
		/* Reaching here means all is good, update state */
		remote_ble_sensor_params.app_state =
			BT_DEMO_APP_STATE_FINDING_SERVICE;

	} else {
		/* In this case a central device connected to us */
		BLE_LOG_INF("Connected central: %s", log_strdup(addr));
		rc = bt_conn_security(conn, BT_SECURITY_MEDIUM);
		if (rc) {
			BLE_LOG_ERR("Failed to set security (%d)", rc);
		}
	}

	return;
fail:
	bt_conn_unref(conn);
	if (conn == sensor_conn) {
		BLE_LOG_ERR("Failed to connect to sensor %s (%u)",
			    log_strdup(addr), err);
		sensor_conn = NULL;
		/* Set state to searching */
		remote_ble_sensor_params.app_state =
			BT_DEMO_APP_STATE_FINDING_DEVICE;

		/* Restart scanning */
		bt_scan();
	} else {
		BLE_LOG_ERR("Failed to connect to central %s (%u)",
			    log_strdup(addr), err);
	}

	return;
}

/* This callback is triggered when a BLE disconnection occurs */
void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	bt_conn_unref(conn);
	if (conn == sensor_conn) {
		BLE_LOG_INF("Disconnected sensor: %s (reason %u)",
			    log_strdup(addr), reason);

		sensor_conn = NULL;

		/* Set state to searching */
		remote_ble_sensor_params.app_state =
			BT_DEMO_APP_STATE_FINDING_DEVICE;

		/* Restart scanning */
		bt_scan();
	} else {
		BLE_LOG_INF("Disconnected central: %s (reason %u)",
			    log_strdup(addr), reason);
	}
}

/* Function for starting BLE scan */
void bt_scan(void)
{
	u8_t err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (err) {
		BLE_LOG_ERR("BLE Scan failed to start (err %d)", err);
		return;
	}

	BLE_LOG_INF("Scanning for remote BLE devices started");
}

static int zephyr_settings_fw_load(struct settings_store *cs,
				   const char *subtree)
{
#if defined(CONFIG_BT_GATT_DIS_SETTINGS)

#if defined(CONFIG_BT_GATT_DIS_FW_REV)
	settings_runtime_set("bt/dis/fw", KERNEL_VERSION_STRING,
			     sizeof(KERNEL_VERSION_STRING));
#endif

#if defined(CONFIG_BT_GATT_DIS_SW_REV)
	settings_runtime_set("bt/dis/sw", APP_VERSION_STRING,
			     sizeof(APP_VERSION_STRING));
#endif

#endif
	return 0;
}

int settings_backend_init(void)
{
	settings_src_register(&zephyr_settings_fw_store);
	return 0;
}

/* Function for initialising the BLE portion of the OOB demo */
void oob_ble_initialise(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		BLE_LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	BLE_LOG_INF("Bluetooth initialized");

	/* Load settings.
    *  DIS uses settings framework to set some charateristics at runtime.
    */
	settings_load();

	bt_conn_cb_register(&conn_callbacks);

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		BLE_LOG_ERR("Advertising failed to start (%d)", err);
	}

	/* Initialize the state to 'looking for device' */
	remote_ble_sensor_params.app_state = BT_DEMO_APP_STATE_FINDING_DEVICE;

	/* Start scanning for Bluetooth devices */
	bt_scan();
}

/* Function for setting the sensor read callback function */
void oob_ble_set_callback(sensor_updated_function_t func)
{
	SensorCallbackFunction = func;
}
