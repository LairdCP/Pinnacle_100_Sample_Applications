#ifndef APP_BT_H
#define APP_BT_H

#include <board.h>

#define ADVERT_BUTTON_PORT      SW1_GPIO_CONTROLLER
#define ADVERT_BUTTON           SW1_GPIO_PIN
#define ADVERT_LED_PORT         LED2_GPIO_CONTROLLER
#define ADVERT_LED              LED2_GPIO_PIN
#define LED_OFF     0
#define LED_ON      1

void app_bt_thread();

#endif