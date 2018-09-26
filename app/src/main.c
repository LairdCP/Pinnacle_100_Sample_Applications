/* main.c - Application main entry point */

#include <zephyr.h>
#include <misc/printk.h>

#include "app_led.h"


void main(void)
{
    printk("Newcastle Test App\n");
    led_init();

    /* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
    while (1)
    {
        ledHeartBeat();
        k_sleep(MSEC_PER_SEC);
        //printk("Heartbeat!\n");
    }
}
