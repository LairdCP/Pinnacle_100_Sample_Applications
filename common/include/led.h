/*
 * Copyright (c) 2020-2023 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LED_H
#define LED_H

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/

#include <lcz_led.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/* Global Constants, Macros and Type Definitions                                                  */
/**************************************************************************************************/
enum led_index {
	BLUE_LED = 0,
	GREEN_LED,
	RED_LED,
#ifdef CONFIG_BOARD_PINNACLE_100_DVK
	GREEN_LED2,
#endif
};

#if defined(CONFIG_BOARD_PINNACLE_100_DVK)
#define NUM_LEDS 4
#elif defined(CONFIG_BOARD_MG100)
#define NUM_LEDS 3
#else
#error "Undefined board"
#endif

/* clang-format off */
#if (NUM_LEDS >= 1)
#define LED1_NODE DT_ALIAS(led0)
#define LED1_DEV 	DEVICE_DT_GET(DT_GPIO_CTLR(LED1_NODE, gpios))
#define LED1_FLAGS 	DT_GPIO_FLAGS(LED1_NODE, gpios)
#define LED1 		DT_GPIO_PIN(LED1_NODE, gpios)
#else
#error "No LEDs defined"
#endif

#if (NUM_LEDS >= 2)
#define LED2_NODE DT_ALIAS(led1)
#define LED2_DEV	DEVICE_DT_GET(DT_GPIO_CTLR(LED2_NODE, gpios))
#define LED2_FLAGS 	DT_GPIO_FLAGS(LED2_NODE, gpios)
#define LED2 		DT_GPIO_PIN(LED2_NODE, gpios)
#endif

#if (NUM_LEDS >= 3)
#define LED3_NODE DT_ALIAS(led2)
#define LED3_DEV 	DEVICE_DT_GET(DT_GPIO_CTLR(LED3_NODE, gpios))
#define LED3_FLAGS 	DT_GPIO_FLAGS(LED3_NODE, gpios)
#define LED3 		DT_GPIO_PIN(LED3_NODE, gpios)
#endif

#if (NUM_LEDS >= 4)
#define LED4_NODE DT_ALIAS(led3)
#define LED4_DEV 	DEVICE_DT_GET(DT_GPIO_CTLR(LED4_NODE, gpios))
#define LED4_FLAGS 	DT_GPIO_FLAGS(LED4_NODE, gpios)
#define LED4 		DT_GPIO_PIN(LED4_NODE, gpios)
#endif
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* LED_H */
