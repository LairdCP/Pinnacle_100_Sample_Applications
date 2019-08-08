/* ble_cellular_service.c - BLE Cellular Service
 *
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_INF
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

static char imei_value[IMEI_LENGTH + 1];

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

/* Cellular Service Declaration */
BT_GATT_SERVICE_DEFINE(cell_svc, BT_GATT_PRIMARY_SERVICE(&cell_svc_uuid),
		       BT_GATT_CHARACTERISTIC(&cell_imei_uuid.uuid,
					      BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, read_imei,
					      NULL, imei_value));

void cell_svc_set_imei(const char *imei)
{
	memcpy(imei_value, imei, sizeof(imei_value) - 1);
}