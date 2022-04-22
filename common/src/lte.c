/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(app_lte);

#include <zephyr.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <net/socket.h>
#ifdef CONFIG_SHELL
#include <shell/shell.h>
#endif
#ifdef CONFIG_SHELL_BACKEND_SERIAL
#include <shell/shell_uart.h>
#endif

#include <drivers/modem/hl7800.h>

#include "lte.h"
#include "led.h"

/******************************************************************************/
/* Global Data Definitions                                                    */
/******************************************************************************/
struct mdm_hl7800_apn *lte_apn_config;

/******************************************************************************/
/* Local Constant, Macro and Type Definitions                                 */
/******************************************************************************/

#ifdef CONFIG_LOG
#define LTE_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define LTE_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define LTE_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define LTE_LOG_DBG(...) LOG_DBG(__VA_ARGS__)
#else
#define LTE_LOG_ERR(...)                                                       \
	{                                                                      \
		printk(__VA_ARGS__);                                           \
		printk("\n");                                                  \
	}
#define LTE_LOG_WRN(...)                                                       \
	{                                                                      \
		printk(__VA_ARGS__);                                           \
		printk("\n");                                                  \
	}
#define LTE_LOG_INF(...)                                                       \
	{                                                                      \
		printk(__VA_ARGS__);                                           \
		printk("\n");                                                  \
	}
#define LTE_LOG_DBG(...)                                                       \
	{                                                                      \
		printk(__VA_ARGS__);                                           \
		printk("\n");                                                  \
	}
#endif

struct mgmt_events {
	uint32_t event;
	net_mgmt_event_handler_t handler;
	struct net_mgmt_event_callback cb;
};

/******************************************************************************/
/* Local Data Definitions                                                     */
/******************************************************************************/
static struct net_if *iface;
static struct net_if_config *cfg;
#ifdef CONFIG_DNS_RESOLVER
static struct dns_resolve_context *dns;
#endif
static struct lte_status lteStatus;
static lte_event_function_t lteCallbackFunction = NULL;

#define NETWORK_SEARCH_PATTERN_ON_MS 100
#define NETWORK_SEARCH_PATTERN_OFF_MS 900

static const struct led_blink_pattern NETWORK_SEARCH_LED_PATTERN = {
	.on_time = NETWORK_SEARCH_PATTERN_ON_MS,
	.off_time = NETWORK_SEARCH_PATTERN_OFF_MS,
	.repeat_count = REPEAT_INDEFINITELY
};

static void onLteEvent(enum lte_event event)
{
	if (lteCallbackFunction != NULL) {
		lteCallbackFunction(event);
	}
}

static void iface_ready_evt_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
#ifdef CONFIG_DNS_RESOLVER
	if (mgmt_event != NET_EVENT_DNS_SERVER_ADD) {
		return;
	}
	LTE_LOG_DBG("LTE DNS addr added!");
#else
	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}
	LTE_LOG_DBG("LTE IP addr added!");
#endif
	led_turn_on(RED_LED);
	onLteEvent(LTE_EVT_READY);
}

static void iface_down_evt_handler(struct net_mgmt_event_callback *cb,
				   uint32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IF_DOWN) {
		return;
	}

	LTE_LOG_DBG("LTE is down");
	led_turn_off(RED_LED);
	onLteEvent(LTE_EVT_DISCONNECTED);
}

static struct mgmt_events iface_events[] = {
	{
#ifdef CONFIG_DNS_RESOLVER
		.event = NET_EVENT_DNS_SERVER_ADD,
#else
		.event = NET_EVENT_IPV4_ADDR_ADD,
#endif
		.handler = iface_ready_evt_handler },
	{ .event = NET_EVENT_IF_DOWN, .handler = iface_down_evt_handler },
	{ 0 } // The for loop below requires this extra location.
};

static void setup_iface_events(void)
{
	int i;

	for (i = 0; iface_events[i].event; i++) {
		net_mgmt_init_event_callback(&iface_events[i].cb,
					     iface_events[i].handler,
					     iface_events[i].event);

		net_mgmt_add_event_callback(&iface_events[i].cb);
	}
}

/* Site survey is initiated by modem shell */
static void site_survey_handler(void *event_data)
{
#ifdef CONFIG_SHELL_BACKEND_SERIAL
	struct mdm_hl7800_site_survey *p = event_data;
	const struct shell *shell = shell_backend_uart_get_ptr();

	if (p != NULL) {
		shell_print(shell, "EARFCN: %u", p->earfcn);
		shell_print(shell, "Cell Id: %u", p->cell_id);
		shell_print(shell, "RSRP: %d", p->rsrp);
		shell_print(shell, "RSRQ: %d", p->rsrq);
	}
#else
	ARG_UNUSED(event_data);
#endif
}

static void modemEventCallback(enum mdm_hl7800_event event, void *event_data)
{
	uint8_t code = ((struct mdm_hl7800_compound_event *)event_data)->code;

	switch (event) {
	case HL7800_EVENT_NETWORK_STATE_CHANGE:
		LTE_LOG_DBG("HL7800 network event: %d", code);
		switch (code) {
		case HL7800_HOME_NETWORK:
		case HL7800_ROAMING:
			led_turn_on(RED_LED);
			break;

		case HL7800_NOT_REGISTERED:
		case HL7800_SEARCHING:
			led_blink(RED_LED, &NETWORK_SEARCH_LED_PATTERN);
			break;

		case HL7800_REGISTRATION_DENIED:
		case HL7800_UNABLE_TO_CONFIGURE:
		case HL7800_OUT_OF_COVERAGE:
		case HL7800_EMERGENCY:
		default:
			led_turn_off(RED_LED);
			onLteEvent(LTE_EVT_DISCONNECTED);
			break;
		}
		break;
	case HL7800_EVENT_STARTUP_STATE_CHANGE:
		switch (code) {
		case HL7800_STARTUP_STATE_READY:
		case HL7800_STARTUP_STATE_WAITING_FOR_ACCESS_CODE:
			break;
		case HL7800_STARTUP_STATE_SIM_NOT_PRESENT:
		case HL7800_STARTUP_STATE_SIMLOCK:
		case HL7800_STARTUP_STATE_UNRECOVERABLE_ERROR:
		case HL7800_STARTUP_STATE_UNKNOWN:
		case HL7800_STARTUP_STATE_INACTIVE_SIM:
		default:
			led_turn_off(RED_LED);
			break;
		}
		break;

	case HL7800_EVENT_SLEEP_STATE_CHANGE:
		switch (code) {
		case HL7800_SLEEP_SLEEP:
			LTE_LOG_DBG("HL7800 asleep");
			break;
		case HL7800_SLEEP_AWAKE:
			LTE_LOG_DBG("HL7800 awake!");
			break;
		case HL7800_SLEEP_HIBERNATE:
			LTE_LOG_DBG("HL7800 hibernate!");
			break;
		case HL7800_SLEEP_LITE_HIBERNATE:
			LTE_LOG_DBG("HL7800 lite hibernate!");
			break;
		default:
			break;
		}
		break;

	case HL7800_EVENT_APN_UPDATE:
		/* event data points to static data stored in modem driver.
		 * Store the pointer so we can access the APN elsewhere in our app.
		 */
		lte_apn_config = (struct mdm_hl7800_apn *)event_data;
		break;
	case HL7800_EVENT_RSSI:
	case HL7800_EVENT_SINR:
	case HL7800_EVENT_RAT:
	case HL7800_EVENT_BANDS:
	case HL7800_EVENT_ACTIVE_BANDS:
	case HL7800_EVENT_FOTA_STATE:
	case HL7800_EVENT_FOTA_COUNT:
	case HL7800_EVENT_REVISION:
		break;
#ifdef CONFIG_MODEM_HL7800_GPS
	case HL7800_EVENT_GPS:
	case HL7800_EVENT_GPS_POSITION_STATUS:
		break;
#endif
#if CONFIG_MODEM_HL7800_POLTE
	case HL7800_EVENT_POLTE_REGISTRATION:
	case HL7800_EVENT_POLTE_LOCATE_STATUS:
	case HL7800_EVENT_POLTE:
		break;
#endif
	case HL7800_EVENT_SITE_SURVEY:
		site_survey_handler(event_data);
		break;
	default:
		LOG_WRN("Unknown modem event");
		break;
	}
}

void lteRegisterEventCallback(lte_event_function_t callback)
{
	lteCallbackFunction = callback;
}

int lteInit(void)
{
	int rc = LTE_ERR_NONE;
	mdm_hl7800_register_event_callback(modemEventCallback);
	setup_iface_events();

	/* wait for network interface to be ready */
	iface = net_if_get_default();
	if (!iface) {
		LTE_LOG_ERR("Could not get iface");
		rc = LTE_ERR_NO_IFACE;
		goto exit;
	}

	cfg = net_if_get_config(iface);
	if (!cfg) {
		LTE_LOG_ERR("Could not get iface config");
		rc = LTE_ERR_IFACE_CFG;
		goto exit;
	}

#ifdef CONFIG_DNS_RESOLVER
	dns = dns_resolve_get_default();
	if (!dns) {
		LTE_LOG_ERR("Could not get DNS context");
		rc = LTE_ERR_DNS_CFG;
		goto exit;
	}
#endif

	lteStatus.radio_version = (const char *)mdm_hl7800_get_fw_version();
	lteStatus.IMEI = (const char *)mdm_hl7800_get_imei();
	lteStatus.ICCID = (const char *)mdm_hl7800_get_iccid();
	lteStatus.serialNumber = (const char *)mdm_hl7800_get_sn();
	mdm_hl7800_generate_status_events();

exit:
	return rc;
}

bool lteIsReady(void)
{
	bool ready = false;
#ifdef CONFIG_DNS_RESOLVER
#if defined(CONFIG_NET_IPV4)
	struct sockaddr_in *dnsAddr;
#elif defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 *dnsAddr;
#endif

	if (iface != NULL && cfg != NULL && &dns->servers[0] != NULL) {
#if defined(CONFIG_NET_IPV4)
		dnsAddr = net_sin(&dns->servers[0].dns_server);
		ready = net_if_is_up(iface) && cfg->ip.ipv4 &&
			!net_ipv4_is_addr_unspecified(&dnsAddr->sin_addr);
#elif defined(CONFIG_NET_IPV6)
		dnsAddr = net_sin6(&dns->servers[0].dns_server);
		ready = net_if_is_up(iface) && cfg->ip.ipv6 &&
			!net_ipv6_is_addr_unspecified(&dnsAddr->sin6_addr);
#endif
	}
#else
	if (iface != NULL && cfg != NULL) {
		ready = net_if_is_up(iface) && cfg->ip.ipv4;
	}
#endif /* CONFIG_DNS_RESOLVER */
	return ready;
}

struct lte_status *lteGetStatus(void)
{
	mdm_hl7800_get_signal_quality(&lteStatus.rssi, &lteStatus.sinr);
	return &lteStatus;
}
