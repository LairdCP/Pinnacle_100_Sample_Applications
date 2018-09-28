/* main.c - Application main entry point */

#include <zephyr.h>
#include <misc/printk.h>

#include "app_led.h"


void main(void)
{
    printk("Newcastle Test App\nSystem up time %d us\n", SYS_CLOCK_HW_CYCLES_TO_NS(k_cycle_get_32())/1000);
    led_init();

    while (1)
    {
        // TODO: LED heartbeat should use a timer instead of relying on the thread loop.
        // TODO: wait in main thread on event (semaphore) until some work needs to be done.
        ledHeartBeat();
        k_sleep(MSEC_PER_SEC);
        //printk("Heartbeat!\n");
    }
}
