#include <zephyr.h>
#include <power.h>
#include <string.h>
#include <soc.h>
#include <device.h>
#include <misc/printk.h>

#include "led.h"
#include "lte.h"

#define BUSY_WAIT_TIME_SECONDS 5
#define BUSY_WAIT_TIME (BUSY_WAIT_TIME_SECONDS * 1000000)
#define SLEEP_TIME_SECONDS 30

#ifdef CONFIG_MODEM_HL7800
K_SEM_DEFINE(lte_event_sem, 0, 1);

static enum lte_event lte_status = LTE_EVT_DISCONNECTED;

static void lteEvent(enum lte_event event)
{
	k_sem_give(&lte_event_sem);
	lte_status = event;
}
#endif /* CONFIG_MODEM_HL7800 */

/* Application main Thread */
void main(void)
{
	printk("\n\n*** Power Management Demo on %s ***\n", CONFIG_BOARD);

	led_init();

#ifdef CONFIG_MODEM_HL7800
	/* init LTE */
	lteRegisterEventCallback(lteEvent);
	int rc = lteInit();
	if (rc < 0) {
		printk("LTE init (%d)\n", rc);
		goto exit;
	}

	if (!lteIsReady()) {
		while (lte_status != LTE_EVT_READY) {
			/* Wait for LTE read evt */
			k_sem_reset(&lte_event_sem);
			printk("Waiting for LTE to be ready...\n");
			k_sem_take(&lte_event_sem, K_FOREVER);
		}
	}
	printk("LTE ready!\n");
#endif /* CONFIG_MODEM_HL7800 */

	while (1) {
#ifdef CONFIG_MODEM_HL7800
		printk("App waiting for event\n");
		k_sem_take(&lte_event_sem, K_FOREVER);
#endif /* CONFIG_MODEM_HL7800 */
		printk("App busy waiting for %d seconds\n",
		       BUSY_WAIT_TIME_SECONDS);
		k_busy_wait(BUSY_WAIT_TIME);
#ifndef CONFIG_MODEM_HL7800
		printk("App sleeping for %d seconds\n", SLEEP_TIME_SECONDS);
		k_sleep(K_SECONDS(SLEEP_TIME_SECONDS));
#endif /* not CONFIG_MODEM_HL7800 */
	}
#ifdef CONFIG_MODEM_HL7800
exit:
#endif /* CONFIG_MODEM_HL7800 */
	printk("Exiting main()\n");
	return;
}