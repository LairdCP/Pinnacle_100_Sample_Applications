#ifndef APP_BT_H
#define APP_BT_H

#include <board.h>

#define ADVERT_BUTTON SW0_GPIO_PIN
#define ADVERT_LED LED1_GPIO_PIN
#define LED_OFF 1
#define LED_ON 0

void app_bt_thread();

#endif