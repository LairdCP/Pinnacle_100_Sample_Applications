/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BLE_CELLULAR_SERVICE_H__
#define __BLE_CELLULAR_SERVICE_H__

enum cell_status { CELL_STATUS_DISCONNECTED, CELL_STATUS_CONNECTED };

void cell_svc_set_imei(const char *imei);
void cell_svc_set_status(enum cell_status status);
void cell_svc_notify_status(struct bt_conn *conn, enum cell_status status);
void cell_svc_set_fw_ver(const char *ver);

#endif