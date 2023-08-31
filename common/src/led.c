/**
 * @file led.c
 *
 * Copyright (c) 2023 Laird Connectivity LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include "led.h"

/**************************************************************************************************/
/* Local Constant, Macro and Type Definitions                                                     */
/**************************************************************************************************/
#define INIT_PRIORITY 99

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/
static int led_init(void)
{
	/* clang-format off */
#if defined(CONFIG_BOARD_MG100)
    struct lcz_led_configuration c[] = {
        { BLUE_LED,   LED2_DEV, LED2, LED2_FLAGS },
        { GREEN_LED,  LED3_DEV, LED3, LED3_FLAGS },
        { RED_LED,    LED1_DEV, LED1, LED1_FLAGS },
    };
#elif defined(CONFIG_BOARD_PINNACLE_100_DVK)
    struct lcz_led_configuration c[] = {
        { BLUE_LED,   LED1_DEV, LED1, LED1_FLAGS },
        { GREEN_LED,  LED2_DEV, LED2, LED2_FLAGS },
        { RED_LED,    LED3_DEV, LED3, LED3_FLAGS },
        { GREEN_LED2, LED4_DEV, LED4, LED4_FLAGS }
    };
#else
#error "Unsupported board selected"
#endif
	/* clang-format on */
	lcz_led_init(c, ARRAY_SIZE(c));

    return 0;
}
/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
SYS_INIT(led_init, APPLICATION, INIT_PRIORITY);
