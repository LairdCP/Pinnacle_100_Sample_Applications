#include <zephyr.h>
#include <misc/printk.h>
#include <string.h>
#include <gpio.h>
#include <uart.h>

#include "hl7800.h"

static struct device *gpio0_dev;
static struct device *gpio1_dev;
static struct device *lte_uart_dev;

static int lteResetState = 0;
static int lteWakeState = 0;
static u8_t lte_uart_rx_buf[LTE_UART_RX_READ_SIZE];
static u8_t lte_rx_msg_buf[LTE_UART_RX_PIPE_SIZE];
static char lte_temp_msg[LTE_UART_RX_PIPE_SIZE];
static int lte_tmp_msg_index = 0;
static int parseCmdState = CMD_STATE_FIND_MSG_START_1;
static bool lteBooted = false;
static int lteBytesRxd = 0;
static int lteBytesProc = 0;

K_PIPE_DEFINE(lte_rx_pipe, LTE_UART_RX_PIPE_SIZE, 1);
K_SEM_DEFINE(lte_rx_sem, 0, 1);
K_SEM_DEFINE(lte_boot_sem, 0, 1);
K_SEM_DEFINE(lte_event_sem, 0, 1);
K_SEM_DEFINE(lte_rx_cmd_sem, 0, 1);

static void initIO();
static void lteUartInit();
static void lte_uart_isr(struct device *dev);
void toggleLteReset();
void toggleLteWake();
static void process_lte_rx();
static void parse_lte_cmd(u8_t *msg, int size);
static int addToLteTempMsg(char *bytes, int length);
static void start_lte_rx_timer();
static void stop_lte_rx_timer();
static void start_lte_boot_timer();
static void stop_lte_boot_timer();
static void lte_rx_timer_handler(struct k_timer *timer_id);
static void lte_boot_timer_handler(struct k_timer *timer_id);
void sendLteCmd(char *cmd);
void waitForLteCmdResponse(s32_t timeout);
static void queryLteInfo();
static void setLteUartRts(bool high);
static void hl7800_thread();
static void hl7800_rx_thread();

K_TIMER_DEFINE(lte_rx_timer, lte_rx_timer_handler, NULL);
K_TIMER_DEFINE(lte_boot_timer, lte_boot_timer_handler, NULL);

K_THREAD_DEFINE(hl7800rx, 512, hl7800_rx_thread, NULL, NULL, NULL,
                6, 0, K_NO_WAIT);

K_THREAD_DEFINE(hl7800, 512, hl7800_thread, NULL, NULL, NULL,
                7, 0, K_NO_WAIT);

// init the control IO for the HL7800
static void initIO()
{
    int ret;
    int pinState;

    // Open the GPIO1 device
    gpio1_dev = device_get_binding(SIO_LTE_IO_CONTROLLER);
    if (!gpio1_dev)
    {
        printk("Cannot find %s!\n", SIO_LTE_IO_CONTROLLER);
        return;
    }

    // Open the GPIO0 device
    gpio0_dev = device_get_binding(SIO_LTE_UART_IO_CONTROLLER);
    if (!gpio0_dev)
    {
        printk("Cannot find %s!\n", SIO_LTE_UART_IO_CONTROLLER);
        return;
    }

    // LTE reset
    ret = gpio_pin_configure(gpio1_dev, SIO_LTE_RESET, (GPIO_DIR_OUT | GPIO_DS_DISCONNECT_HIGH));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ret);
    }
    lteResetState = 1;
    ret = gpio_pin_write(gpio1_dev, SIO_LTE_RESET, lteResetState);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", ret);
    }
    toggleLteReset();
    k_sleep(50);
    toggleLteReset();

    // LTE wake
    ret = gpio_pin_configure(gpio1_dev, SIO_LTE_WAKE, (GPIO_DIR_OUT | GPIO_DS_DISCONNECT_LOW));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ret);
    }
    lteWakeState = 1;
    ret = gpio_pin_write(gpio1_dev, SIO_LTE_WAKE, lteWakeState);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", ret);
    }
    else
    {
        printk("SIO_LTE_WAKE set to %d\n", lteWakeState);
    }
    // ret = gpio_pin_configure(gpio1_dev, SIO_LTE_WAKE, GPIO_DIR_IN);
    // if (ret)
    // {
    //     printk("Error configuring GPIO %d!\n", ret);
    // }
    // else
    // {
    //     printk("SIO_LTE_WAKE config as input\n");
    // }

    // LTE pwr on
    ret = gpio_pin_configure(gpio1_dev, SIO_LTE_POWER_ON, (GPIO_DIR_OUT | GPIO_DS_DISCONNECT_HIGH));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ret);
    }
    pinState = 1;
    ret = gpio_pin_write(gpio1_dev, SIO_LTE_POWER_ON, pinState);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", ret);
    }
    else
    {
        printk("SIO_LTE_POWER_ON set to %d\n", pinState);
    }
    // ret = gpio_pin_configure(gpio1_dev, SIO_LTE_POWER_ON, GPIO_DIR_IN);
    // if (ret)
    // {
    //     printk("Error configuring GPIO %d!\n", ret);
    // }
    // else
    // {
    //     printk("SIO_LTE_POWER_ON config as input\n");
    // }

    // LTE fast shutdown
    ret = gpio_pin_configure(gpio1_dev, SIO_LTE_FAST_SHUTD, (GPIO_DIR_OUT | GPIO_DS_DISCONNECT_HIGH));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ret);
    }
    pinState = 1;
    ret = gpio_pin_write(gpio1_dev, SIO_LTE_FAST_SHUTD, pinState);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", ret);
    }
    else
    {
        printk("SIO_LTE_FAST_SHUTD set to %d\n", pinState);
    }
    // ret = gpio_pin_configure(gpio1_dev, SIO_LTE_FAST_SHUTD, GPIO_DIR_IN);
    // if (ret)
    // {
    //     printk("Error configuring GPIO %d!\n", ret);
    // }
    // else
    // {
    //     printk("SIO_LTE_FAST_SHUTD config as input\n");
    // }

    // LTE gps enable
    ret = gpio_pin_configure(gpio1_dev, SIO_LTE_GPS_EN, GPIO_DIR_IN);
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ret);
    }
    else
    {
        printk("SIO_LTE_GPS_EN config as input\n");
    }

    // LTE TX on
    ret = gpio_pin_configure(gpio1_dev, SIO_LTE_TX_ON, GPIO_DIR_IN);
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ret);
    }
    else
    {
        printk("SIO_LTE_TX_ON config as input\n");
    }

    // LTE UART CTS
    ret = gpio_pin_configure(gpio0_dev, SIO_LTE_CTS, GPIO_DIR_IN);
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ret);
    }
    else
    {
        printk("SIO_LTE_CTS config as input\n");
    }

    // LTE UART RTS
    ret = gpio_pin_configure(gpio0_dev, SIO_LTE_RTS, GPIO_DIR_OUT);
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ret);
    }
    pinState = 0;
    ret = gpio_pin_write(gpio0_dev, SIO_LTE_RTS, pinState);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", ret);
    }
    else
    {
        printk("SIO_LTE_RTS set to %d\n", pinState);
    }
}

static void lteUartInit()
{
    lte_uart_dev = device_get_binding(LTE_UART_DEV);
    if (lte_uart_dev == NULL)
    {
        printk("Error opening LTE UART\n");
        return;
    }

    uart_irq_callback_set(lte_uart_dev, lte_uart_isr);
    uart_irq_rx_enable(lte_uart_dev);
}

static void lte_uart_isr(struct device *dev)
{
    int len, ret;
    size_t bytes_written;

    /* Verify uart_irq_update() */
    if (!uart_irq_update(dev))
    {
        printk("retval should always be 1\n");
        return;
    }

    while (uart_irq_update(dev) &&
           uart_irq_rx_ready(dev))
    {
        len = uart_fifo_read(lte_uart_dev, lte_uart_rx_buf, sizeof(lte_uart_rx_buf));

        if (len > 0)
        {
            lteBytesRxd += len;
            ret = k_pipe_put(&lte_rx_pipe,
                             lte_uart_rx_buf,
                             len, &bytes_written,
                             len, K_NO_WAIT);
            if (ret < 0)
            {
                printk("UART pip write error %d\n", ret);
            }
            else
            {
                k_sem_give(&lte_rx_sem);
            }
        }
    }
}

void toggleLteReset()
{
    int ret;

    if (lteResetState)
    {
        // set it low
        ret = gpio_pin_write(gpio1_dev, SIO_LTE_RESET, 0);
        printk("t:%d LTE reset is low\n", k_uptime_get_32());
        lteResetState = 0;
        lteBooted = false;
        stop_lte_boot_timer();
    }
    else
    {
        // set it high
        ret = gpio_pin_write(gpio1_dev, SIO_LTE_RESET, 1);
        printk("t:%d LTE reset is high\n", k_uptime_get_32());
        lteResetState = 1;
        start_lte_boot_timer();
    }
}

void toggleLteWake()
{
    int ret;

    if (lteWakeState)
    {
        // set it low
        ret = gpio_pin_write(gpio1_dev, SIO_LTE_WAKE, 0);
        printk("t:%d LTE wake is low\n", k_uptime_get_32());
        lteWakeState = 0;
    }
    else
    {
        // set it high
        ret = gpio_pin_write(gpio1_dev, SIO_LTE_WAKE, 1);
        printk("t:%d LTE wake is high\n", k_uptime_get_32());
        lteWakeState = 1;
    }
}

static void process_lte_rx()
{
    int rc;
    size_t bytesRead;
    rc = k_sem_take(&lte_rx_sem, K_FOREVER);
    do
    {
        rc = k_pipe_get(&lte_rx_pipe,
                        lte_rx_msg_buf,
                        sizeof(lte_rx_msg_buf),
                        &bytesRead, 1, 1);
        if (rc >= 0)
        {
            parse_lte_cmd(lte_rx_msg_buf, bytesRead);
        }
    } while (rc >= 0);
}

// TODO: evaluate how to frame RX messages properly. Commented out code for framing RX messages.
static void parse_lte_cmd(u8_t *msg, int size)
{
    int index = 0;
    int cpySize = 0;
    lteBytesProc += size;
    do
    {
        switch (parseCmdState)
        {
        case CMD_STATE_FIND_MSG_START_1:
            printk("t:%d LTE msg:\n", k_uptime_get_32());
            lte_tmp_msg_index = 0;
            parseCmdState = CMD_STATE_GET_REST;
            break;
        case CMD_STATE_GET_REST:
            // Continue receiving message
            stop_lte_rx_timer();
            cpySize = size - index;
            int bytesAdded = addToLteTempMsg(&msg[index], cpySize);
            index += bytesAdded;
            if (bytesAdded != cpySize)
            {
                // could not add everything because buffer is full
                // print what we have and start over
                setLteUartRts(true); // Disable LTE UART RX (software flow control)
                printk("%s", lte_temp_msg);
                // "clear" the buffer by starting at index 0
                lte_tmp_msg_index = 0;
                setLteUartRts(false); // Allow LTE UART RX again
            }
            else
            {
                start_lte_rx_timer();
            }
            break;
        };
    } while (index < size);
}

static int addToLteTempMsg(char *bytes, int length)
{
    int bytesAdded = 0;
    int end = lte_tmp_msg_index + length;
    int bufferSize = sizeof(lte_temp_msg);

    if (lte_tmp_msg_index >= bufferSize)
    {
        return bytesAdded;
    }

    if (end > bufferSize - 1)
    {
        int bytesToAdd = bufferSize - lte_tmp_msg_index - 1; // add one less byte so the string is null terminated
        strncpy(&lte_temp_msg[lte_tmp_msg_index], bytes, bytesToAdd);
        lte_tmp_msg_index += bytesToAdd;
        bytesAdded = bytesToAdd;
    }
    else
    {
        strncpy(&lte_temp_msg[lte_tmp_msg_index], bytes, length);
        lte_tmp_msg_index += length;
        bytesAdded = length;
    }

    return bytesAdded;
}

static void start_lte_rx_timer()
{
    k_timer_start(&lte_rx_timer, K_MSEC(LTE_UART_RX_TIMEOUT), 0);
}

static void stop_lte_rx_timer()
{
    k_timer_stop(&lte_rx_timer);
}

static void start_lte_boot_timer()
{
    k_timer_start(&lte_boot_timer, K_SECONDS(LTE_BOOT_TIME), 0);
}

static void stop_lte_boot_timer()
{
    k_timer_stop(&lte_boot_timer);
}

static void lte_rx_timer_handler(struct k_timer *timer_id)
{
    u8_t nul = 0;
    addToLteTempMsg(&nul, 1);
    if (strlen(lte_temp_msg) > 0)
    {
        printk("%s\nBytes RXd: %d\nBytes proc: %d\n\n", lte_temp_msg, lteBytesRxd, lteBytesProc);
        if (lteBytesRxd != lteBytesProc)
        {
            printk("\n\n!!!Did not process all RX bytes!!!\n\n");
        }
        lteBytesRxd = 0;
        lteBytesProc = 0;
        k_sem_give(&lte_rx_cmd_sem);
    }
    parseCmdState = CMD_STATE_FIND_MSG_START_1;
}

static void lte_boot_timer_handler(struct k_timer *timer_id)
{
    k_sem_give(&lte_boot_sem);
    lteBooted = true;
}

// TODO: use message queue to send commands to the HL7800 so the sending thread is not blocked
void sendLteCmd(char *cmd)
{
    int sendLen = strlen(cmd);
    if (sendLen > 0)
    {
        printk("t:%d Send LTE msg: %s\n\n", k_uptime_get_32(), cmd);
        k_sem_reset(&lte_rx_cmd_sem);
        for (int i = 0; i < sendLen; i++)
        {
            uart_poll_out(lte_uart_dev, cmd[i]);
        }
    }
}

void waitForLteCmdResponse(s32_t timeout)
{
    k_sem_take(&lte_rx_cmd_sem, timeout);
}

static void queryLteInfo()
{
    sendLteCmd("ati9\r");
    waitForLteCmdResponse(K_FOREVER);
    sendLteCmd("at+kgsn=3\r");
    waitForLteCmdResponse(K_FOREVER);
    sendLteCmd("at+cgsn\r");
    waitForLteCmdResponse(K_FOREVER);
    sendLteCmd("at+ksrep?\r");
    waitForLteCmdResponse(K_FOREVER);
    sendLteCmd("at+ksleep?\r");
    waitForLteCmdResponse(K_FOREVER);
    sendLteCmd("at+cpsms?\r");
    waitForLteCmdResponse(K_FOREVER);
    sendLteCmd("at+kbndcfg?\r");
    waitForLteCmdResponse(K_FOREVER);
}

static void setLteUartRts(bool high)
{
    int ret;

    if (high)
    {
        ret = gpio_pin_write(gpio0_dev, SIO_LTE_RTS, 1);
        if (ret)
        {
            printk("Error setting SIO_LTE_RTS high %d!\n", ret);
        }
    }
    else
    {
        ret = gpio_pin_write(gpio0_dev, SIO_LTE_RTS, 0);
        if (ret)
        {
            printk("Error setting SIO_LTE_RTS low %d!\n", ret);
        }
    }
}

static void hl7800_thread()
{
    initIO();
    lteUartInit();
    k_sem_take(&lte_boot_sem, K_FOREVER);
    printk("LTE booted, time:%d\n", k_uptime_get_32());
    queryLteInfo();

    while (1)
    {
        k_sem_take(&lte_event_sem, K_FOREVER);
        printk("LTE event received, do some work.\n");
    }
}

static void hl7800_rx_thread()
{
    while (1)
    {
        process_lte_rx();
    }
}
