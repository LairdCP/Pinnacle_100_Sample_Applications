/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LED_H
#define LED_H

//=============================================================================
// Global Constants, Macros and Type Definitions
//=============================================================================

enum led_index {
	BLUE_LED1 = 0,
	GREEN_LED2,
	RED_LED3,
	GREEN_LED4, /* not recommended for use - reserved for bootloader */

	NUMBER_OF_LEDS
};

/* The on and off times are in system ticks.
 * If the repeat count is 2 the pattern will be displayed 3 
 * times (repeated twice). 
 */
struct led_blink_pattern {
	s32_t on_time;
	s32_t off_time;
	u32_t repeat_count;
};

#define REPEAT_INDEFINITELY (0xFFFFFFFF)

//=============================================================================
// Board definitions
//=============================================================================

#define LED1_DEV DT_GPIO_LEDS_LED0_GPIO_CONTROLLER
#define LED1 DT_GPIO_LEDS_LED0_GPIO_PIN
#define LED2_DEV DT_GPIO_LEDS_LED1_GPIO_CONTROLLER
#define LED2 DT_GPIO_LEDS_LED1_GPIO_PIN
#define LED3_DEV DT_GPIO_LEDS_LED2_GPIO_CONTROLLER
#define LED3 DT_GPIO_LEDS_LED2_GPIO_PIN
#define LED4_DEV DT_GPIO_LEDS_LED3_GPIO_CONTROLLER
#define LED4 DT_GPIO_LEDS_LED3_GPIO_PIN

//=============================================================================
// Global Data Definitions
//=============================================================================
// NA

//=============================================================================
// Global Function Prototypes
//=============================================================================

/**
 * @brief Init the LEDs for the board.  
 * 
 * LEDs should be off after completion.
 * Creates mutex used by on/off/blink functions.
 *
 */
void led_init(void);

/**
 * @param index is a Valid LED
 */
void led_turn_on(enum led_index index);

void led_turn_on_isr(enum led_index index);

/**
 * @param index is a Valid LED
 */
void led_turn_off(enum led_index index);

void led_turn_off_isr(enum led_index index);

/**
 * @param index is a Valid LED
 * @param led_blink_pattern @ref struct led_blink_pattern
 * 
 * @details
 *  The pattern in copied by the LED driver.
 */
void led_blink(enum led_index index, struct led_blink_pattern const *pPattern);

/**
 * @param param function Called in system work queue context 
 * when pattern is complete (use NULL to disable).
 */
void led_register_pattern_complete_function(enum led_index index,
					    void (*function)(void));

/**
 * @param retval true if a blink pattern is running, false if it is complete
 */
bool led_pattern_busy(enum led_index index);

#endif /* LED_H */
