#ifndef APP_LED_H
#define APP_LED_H

#define LED_OFF 0
#define LED_ON 1

#define LED1_NODE DT_ALIAS(led0)
#define LED2_NODE DT_ALIAS(led1)
#define LED3_NODE DT_ALIAS(led2)
#define LED4_NODE DT_ALIAS(led3)

#define LED1_DEV DT_GPIO_LABEL(LED1_NODE, gpios)
#define LED1 DT_GPIO_PIN(LED1_NODE, gpios)
#define LED2_DEV DT_GPIO_LABEL(LED2_NODE, gpios)
#define LED2 DT_GPIO_PIN(LED2_NODE, gpios)
#define LED3_DEV DT_GPIO_LABEL(LED3_NODE, gpios)
#define LED3 DT_GPIO_PIN(LED3_NODE, gpios)
#define LED4_DEV DT_GPIO_LABEL(LED4_NODE, gpios)
#define LED4 DT_GPIO_PIN(LED4_NODE, gpios)

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