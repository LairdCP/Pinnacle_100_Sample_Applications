/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LTE_H__
#define __LTE_H__

#define LTE_STACK_SIZE 4096
#define LTE_THREAD_PRIORITY K_PRIO_COOP(1)
#define LTE_RETRY_AWS_ACTION_TIMEOUT_SECONDS 10

enum lte_errors { LTE_ERROR_NONE = 0, LTE_ERROR_NOT_CONNECTED = -1 };

int lteSendSensorData(float temperature, float humidity, int pressure);

#endif