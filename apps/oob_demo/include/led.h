#ifndef LED_H
#define LED_H

#define LED_OFF 0
#define LED_ON 1

#define LED1_DEV LED0_GPIO_CONTROLLER
#define LED1 LED0_GPIO_PIN
#define LED2_DEV LED1_GPIO_CONTROLLER
#define LED2 LED1_GPIO_PIN
#define LED3_DEV LED2_GPIO_CONTROLLER
#define LED3 LED2_GPIO_PIN
#define LED4_DEV LED3_GPIO_CONTROLLER
#define LED4 LED3_GPIO_PIN

/**
 *  Init the LEDs for the board
 */
void led_init(void);

#endif /* LED_H */