#include <zephyr.h>
#include <misc/printk.h>
#include <string.h>
#include <gpio.h>
#include <uart.h>

#include "hl7800.h"

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
static void start_lte_rx_timer();
static void stop_lte_rx_timer();
static void start_lte_boot_timer();
static void stop_lte_boot_timer();
static void lte_rx_timer_handler(struct k_timer *timer_id);
static void lte_boot_timer_handler(struct k_timer *timer_id);
void sendLteCmd(char *cmd);
void waitForLteCmdResponse();
static void queryLteInfo();
static void hl7800_thread();
static void hl7800_rx_thread();

K_TIMER_DEFINE(lte_rx_timer, lte_rx_timer_handler, NULL);
K_TIMER_DEFINE(lte_boot_timer, lte_boot_timer_handler, NULL);

K_THREAD_DEFINE(hl7800rx, 512, hl7800_rx_thread, NULL, NULL, NULL,
                7, 0, K_NO_WAIT);

K_THREAD_DEFINE(hl7800, 512, hl7800_thread, NULL, NULL, NULL,
                7, 0, K_NO_WAIT);

// init the control IO for the HL7800
static void initIO()
{
    int ret;

    // Open the GPIO1 device
    gpio1_dev = device_get_binding(SIO_LTE_IO_CONTROLLER);
    if (!gpio1_dev)
    {
        printk("Cannot find %s!\n", SIO_LTE_IO_CONTROLLER);
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
    toggleLteReset();

    // LTE wake
    ret = gpio_pin_configure(gpio1_dev, SIO_LTE_WAKE, GPIO_DIR_OUT);
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

    // LTE pwr on
    ret = gpio_pin_configure(gpio1_dev, SIO_LTE_POWER_ON, (GPIO_DIR_OUT | GPIO_DS_DISCONNECT_HIGH));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ret);
    }
    ret = gpio_pin_write(gpio1_dev, SIO_LTE_POWER_ON, 1);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", ret);
    }

    // LTE fast shutdown
    ret = gpio_pin_configure(gpio1_dev, SIO_LTE_FAST_SHUTD, (GPIO_DIR_OUT | GPIO_DS_DISCONNECT_HIGH));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ret);
    }
    ret = gpio_pin_write(gpio1_dev, SIO_LTE_FAST_SHUTD, 1);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", ret);
    }

    // LTE gps enable
    ret = gpio_pin_configure(gpio1_dev, SIO_LTE_GPS_EN, (GPIO_DIR_OUT | GPIO_DS_DISCONNECT_HIGH));
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ret);
    }
    ret = gpio_pin_write(gpio1_dev, SIO_LTE_GPS_EN, 0);
    if (ret)
    {
        printk("Error setting GPIO %d!\n", ret);
    }

    // LTE TX on
    ret = gpio_pin_configure(gpio1_dev, SIO_LTE_TX_ON, GPIO_DIR_IN);
    if (ret)
    {
        printk("Error configuring GPIO %d!\n", ret);
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
            //printk("Bytes read: %d, msg: %s\n", len, lte_uart_rx_buf);
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
                //printk("%d bytes read; %d bytes added to pipe; bytes: %s\n", len, bytes_written, lte_uart_rx_buf);
                k_sem_give(&lte_rx_sem);
                // printk("Give semaphore\n");
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
        printk("LTE reset is low\n");
        lteResetState = 0;
        lteBooted = false;
        stop_lte_boot_timer();
    }
    else
    {
        // set it high
        ret = gpio_pin_write(gpio1_dev, SIO_LTE_RESET, 1);
        printk("LTE reset is high\n");
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
        printk("LTE wake is low\n");
        lteWakeState = 0;
    }
    else
    {
        // set it high
        ret = gpio_pin_write(gpio1_dev, SIO_LTE_WAKE, 1);
        printk("LTE wake is high\n");
        lteWakeState = 1;
    }
}

static void process_lte_rx()
{
    int rc;
    size_t bytesRead;

    rc = k_sem_take(&lte_rx_sem, K_FOREVER);
    rc = k_pipe_get(&lte_rx_pipe,
                    lte_rx_msg_buf,
                    sizeof(lte_rx_msg_buf),
                    &bytesRead, 1, K_NO_WAIT);
    if (rc >= 0)
    {
        parse_lte_cmd(lte_rx_msg_buf, bytesRead);
    }
}

static void parse_lte_cmd(u8_t *msg, int size)
{
    int index = 0;
    // char okStr[5];
    // char errStr[8];
    int cpySize = 0;
    //printk("LTE msg: %s\n", msg);

    do
    {
        switch (parseCmdState)
        {
        case CMD_STATE_FIND_MSG_START_1:
            lte_tmp_msg_index = 0;
            memset(lte_temp_msg, 0, sizeof(lte_temp_msg));
            if (msg[index] == '\r')
            {
                lte_temp_msg[lte_tmp_msg_index++] = msg[index++];
                parseCmdState = CMD_STATE_FIND_MSG_START_2;
                start_lte_rx_timer();
            }
            else
            {
                // Throw the byte away
                index++;
            }
            break;
        case CMD_STATE_FIND_MSG_START_2:
            if (msg[index] == '\n')
            {
                lte_temp_msg[lte_tmp_msg_index++] = msg[index++];
                parseCmdState = CMD_STATE_GET_REST;
            }
            else
            {
                stop_lte_rx_timer();
                parseCmdState = CMD_STATE_FIND_MSG_START_1;
            }
            break;
        case CMD_STATE_GET_REST:
            // if (size >= 4 && msg[index] == 'O')
            // {
            //     parseCmdState = CMD_STATE_GET_OK;
            // }
            // else if (size >= 7 && msg[index] == 'E')
            // {
            //     parseCmdState = CMD_STATE_GET_ERROR;
            // }
            // else if (size >= 10 &&
            //          msg[index] == '+' &&
            //          msg[index + 1] == 'C' &&
            //          msg[index + 2] == 'M' &&
            //          msg[index + 3] == 'E')
            // {
            //     parseCmdState = CMD_STATE_GET_EXT_ERROR;
            // }
            // else
            {
                // Continue receiving message
                stop_lte_rx_timer();
                cpySize = size - index;
                strncpy(&lte_temp_msg[lte_tmp_msg_index], &msg[index], cpySize);
                lte_tmp_msg_index += cpySize;
                index += cpySize;
                start_lte_rx_timer();
            }
            break;
            // case CMD_STATE_GET_OK:
            //     strncpy(okStr, &msg[index], sizeof(okStr) - 1);
            //     if (strcmp(okStr, "OK\r\n") == 0)
            //     {
            //         stop_lte_rx_timer();
            //         strcpy(&lte_temp_msg[lte_tmp_msg_index], okStr);
            //         index += 4;
            //         // Print the complete message
            //         printk("LTE msg: %s\n", lte_temp_msg);
            //         parseCmdState = CMD_STATE_FIND_MSG_START_1;
            //     }
            //     else
            //     {
            //         parseCmdState = CMD_STATE_GET_REST;
            //     }
            //     break;
            // case CMD_STATE_GET_ERROR:
            //     strncpy(errStr, &msg[index], sizeof(errStr) - 1);
            //     if (strcmp(errStr, "ERROR\r\n") == 0)
            //     {
            //         stop_lte_rx_timer();
            //         strcpy(&lte_temp_msg[lte_tmp_msg_index], errStr);
            //         index += 7;
            //         // Print the complete message
            //         printk("LTE msg: %s\n", lte_temp_msg);
            //         parseCmdState = CMD_STATE_FIND_MSG_START_1;
            //     }
            //     else
            //     {
            //         parseCmdState = CMD_STATE_GET_REST;
            //     }
            //     break;
            // case CMD_STATE_GET_EXT_ERROR:
            //     stop_lte_rx_timer();
            //     parseCmdState = CMD_STATE_FIND_MSG_START_1;
            //     break;
        };
    } while (index < size);
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
    if (strlen(lte_temp_msg) > 0)
    {
        k_sem_give(&lte_rx_cmd_sem);
        printk("LTE msg: %s", lte_temp_msg);
    }
    parseCmdState = CMD_STATE_FIND_MSG_START_1;
}

static void lte_boot_timer_handler(struct k_timer *timer_id)
{
    k_sem_give(&lte_boot_sem);
    lteBooted = true;
}

void sendLteCmd(char *cmd)
{
    if (!lteBooted)
    {
        printk("LTE must be booted to send a message.\n");
        return;
    }

    if (!lteWakeState)
    {
        printk("LTE wake must be high to send a message.\n");
        return;
    }

    int len = strlen(cmd);
    if (len > 0)
    {
        printk("Send LTE msg: %s", cmd);
        k_sem_reset(&lte_rx_cmd_sem);
        for (int i = 0; i < len; i++)
        {
            uart_poll_out(lte_uart_dev, cmd[i]);
        }
    }
}

void waitForLteCmdResponse()
{
    k_sem_take(&lte_rx_cmd_sem, K_MSEC(5000));
}

static void queryLteInfo()
{
    sendLteCmd("ati9\r");
    waitForLteCmdResponse();
    sendLteCmd("at+kgsn=3\r");
    waitForLteCmdResponse();
    sendLteCmd("at+cgsn\r");
    waitForLteCmdResponse();
    sendLteCmd("at+ksrep?\r");
    waitForLteCmdResponse();
    sendLteCmd("at+ksleep?\r");
    waitForLteCmdResponse();
    sendLteCmd("at+cpsms?\r");
    waitForLteCmdResponse();
    sendLteCmd("at+kbndcfg?\r");
    waitForLteCmdResponse();
}

static void hl7800_thread()
{
    initIO();
    lteUartInit();
    k_sem_take(&lte_boot_sem, K_FOREVER);
    printk("LTE booted\n");
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
