#ifndef APP_LED_H
#define APP_LED_H

#include <board.h>

#define LED_OFF 0
#define LED_ON  1
#define HEARTBEAT_LED_PORT  LED1_GPIO_CONTROLLER
#define HEARTBEAT_LED       LED1_GPIO_PIN


/**
 *  Init the LEDs for the board
 */
void led_init(void);

/**
 * This will flash the heart beat LED twice.
 * This function will block for the amount of time it takes to flash the LED
 */
void ledHeartBeat(void);

#endif