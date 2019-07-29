/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NV_H
#define NV_H

#define NV_FLASH_DEVICE DT_FLASH_DEV_NAME
#define NV_FLASH_OFFSET DT_FLASH_AREA_STORAGE_OFFSET

#define NUM_FLASH_SECTORS 3

int nvInit(void);
int nvReadCommissioned(bool *commissioned);
int nvStoreCommissioned(bool commissioned);
int nvStoreDevCert(u8_t *cert, u16_t size);
int nvStoreDevKey(u8_t *key, u16_t size);
int nvReadDevCert(u8_t *cert, u16_t size);
int nvReadDevKey(u8_t *key, u16_t size);
int nvDeleteDevCert(void);
int nvDeleteDevKey(void);

enum SETTING_ID {
	SETTING_ID_COMMISSIONED,
	SETTING_ID_DEV_CERT,
	SETTING_ID_DEV_KEY
};

#endif