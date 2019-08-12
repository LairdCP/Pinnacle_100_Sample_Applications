/* ble_cellular_service.c - BLE Cellular Service
 *
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(oob_cell_svc);

#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "oob_common.h"
#include "ble_cellular_service.h"

#define CELL_SVC_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define CELL_SVC_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define CELL_SVC_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define CELL_SVC_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

static struct bt_uuid_128 cell_svc_uuid =
	BT_UUID_INIT_128(0x36, 0xa3, 0x4d, 0x40, 0xb6, 0x70, 0x69, 0xa6, 0xb1,
			 0x4e, 0x84, 0x9e, 0x60, 0x7c, 0x78, 0x43);

static struct bt_uuid_128 cell_imei_uuid =
	BT_UUID_INIT_128(0x36, 0xa3, 0x4d, 0x40, 0xb6, 0x70, 0x69, 0xa6, 0xb1,
			 0x4e, 0x84, 0x9e, 0x61, 0x7c, 0x78, 0x43);

static struct bt_uuid_128 cell_status_uuid =
	BT_UUID_INIT_128(0x36, 0xa3, 0x4d, 0x40, 0xb6, 0x70, 0x69, 0xa6, 0xb1,
			 0x4e, 0x84, 0x9e, 0x65, 0x7c, 0x78, 0x43);

static struct bt_uuid_128 cell_fw_ver_uuid =
	BT_UUID_INIT_128(0x36, 0xa3, 0x4d, 0x40, 0xb6, 0x70, 0x69, 0xa6, 0xb1,
			 0x4e, 0x84, 0x9e, 0x66, 0x7c, 0x78, 0x43);

static char imei_value[IMEI_LENGTH + 1];
static struct bt_gatt_ccc_cfg status_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t status_notify;
static enum cell_status status_value;
static char fw_ver_value[LTE_FW_VER_LENGTH_MAX + 1];

static ssize_t read_imei(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	u16_t valueLen;

	const char *value = attr->user_data;
	valueLen = strlen(value);
	if (valueLen > IMEI_LENGTH) {
		valueLen = IMEI_LENGTH;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, valueLen);
}

static ssize_t read_status(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(status_value));
}

static void status_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	status_notify = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_fw_ver(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   u16_t len, u16_t offset)
{
	u16_t valueLen;

	const char *value = attr->user_data;
	valueLen = strlen(value);
	if (valueLen > LTE_FW_VER_LENGTH_MAX) {
		valueLen = LTE_FW_VER_LENGTH_MAX;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, valueLen);
}

/* Cellular Service Declaration */
BT_GATT_SERVICE_DEFINE(
	cell_svc, BT_GATT_PRIMARY_SERVICE(&cell_svc_uuid),
	BT_GATT_CHARACTERISTIC(&cell_imei_uuid.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_imei, NULL, imei_value),
	BT_GATT_CHARACTERISTIC(&cell_status_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_status, NULL,
			       &status_value),
	BT_GATT_CCC(status_ccc_cfg, status_cfg_changed),
	BT_GATT_CHARACTERISTIC(&cell_fw_ver_uuid.uuid, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_fw_ver, NULL,
			       fw_ver_value), );

void cell_svc_set_imei(const char *imei)
{
	memcpy(imei_value, imei, sizeof(imei_value) - 1);
}

void cell_svc_set_status(enum cell_status status)
{
	status_value = status;
}

void cell_svc_notify_status(struct bt_conn *conn, enum cell_status status)
{
	bool notify = false;

	if (status != status_value) {
		notify = true;
		cell_svc_set_status(status);
	}

	if (conn == NULL) {
		return;
	}
	if (status_notify && notify) {
		bt_gatt_notify(conn, &cell_svc.attrs[3], &status,
			       sizeof(status));
	}
}

void cell_svc_set_fw_ver(const char *ver)
{
	memcpy(fw_ver_value, ver, sizeof(fw_ver_value) - 1);
}