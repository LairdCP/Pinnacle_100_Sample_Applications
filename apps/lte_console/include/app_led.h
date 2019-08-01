#ifndef APP_LED_H
#define APP_LED_H

#define LED_OFF 0
#define LED_ON  1

#define LED1_DEV DT_GPIO_LEDS_LED_1_GPIOS_CONTROLLER
#define LED1 DT_GPIO_LEDS_LED_1_GPIOS_PIN
#define LED2_DEV DT_GPIO_LEDS_LED_2_GPIOS_CONTROLLER
#define LED2 DT_GPIO_LEDS_LED_2_GPIOS_PIN
#define LED3_DEV DT_GPIO_LEDS_LED_3_GPIOS_CONTROLLER
#define LED3 DT_GPIO_LEDS_LED_3_GPIOS_PIN
#define LED4_DEV DT_GPIO_LEDS_LED_4_GPIOS_CONTROLLER
#define LED4 DT_GPIO_LEDS_LED_4_GPIOS_PIN

#define BLUE_LED LED1
#define BLUE_LED_DEV LED1_DEV
#define GREEN1_LED LED2
#define GREEN1_LED_DEV LED2_DEV
#define RED_LED LED3
#define RED_LED_DEV LED3_DEV
#define GREEN2_LED LED4
#define GREEN2_LED_DEV LED4_DEV

#define HEARTBEAT_LED GREEN1_LED
#define HEARTBEAT_ON_TIME K_MSEC(25)
#define HEARTBEAT_OFF_TIME K_MSEC(50)

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