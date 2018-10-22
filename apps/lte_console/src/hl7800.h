#ifndef HL7800_H
#define HL7800_H

#define SIO_LTE_IO_CONTROLLER "GPIO_1"
#define SIO_LTE_RESET 15      //M2.71
#define SIO_LTE_WAKE 13       //M2.48
#define SIO_LTE_POWER_ON 2    //M2.67
#define SIO_LTE_TX_ON 3       //M2.44
#define SIO_LTE_GPS_EN 12     //M2.46
#define SIO_LTE_FAST_SHUTD 14 //M2.50

#define SIO_LTE_UART_IO_CONTROLLER "GPIO_0"
#define SIO_LTE_CTS 15
#define SIO_LTE_RTS 13

#define LTE_UART_DEV "UART_1"
#define LTE_UART_RX_READ_SIZE 64                    // size in bytes
#define LTE_UART_RX_PIPE_SIZE 1024                  // size in bytes
#define LTE_UART_RX_TIMEOUT 25                      // time in ms
#define LTE_BOOT_TIME 10                            // time in seconds
#define LTE_DEFAULT_RESPONSE_WAIT_TIME K_MSEC(7000) // time in ms

enum LteProcessCmdStates
{
    CMD_STATE_FIND_MSG_START_1,
    CMD_STATE_FIND_MSG_START_2,
    CMD_STATE_GET_REST,
    CMD_STATE_GET_OK,
    CMD_STATE_GET_ERROR,
    CMD_STATE_GET_EXT_ERROR
};

void toggleLteReset();
void toggleLteWake();
void sendLteCmd(char *cmd);
void waitForLteCmdResponse();

#endif