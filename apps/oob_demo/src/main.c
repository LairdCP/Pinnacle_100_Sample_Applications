
/* main.c - Application main entry point */

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

#include "oob_ble.h"

void main(void)
{
	/* Start up BLE portion of the demo */
	oob_ble_initialise();
}
