#ifndef UART_COMM_H_
#define UART_COMM_H_

#include <stdbool.h>
#include <stdint.h>

/*
 * 三路串口编号：
 * CAR_UART_PORT_0 -> UART_0，PA10 TX / PA11 RX，常用于调试或上位机。
 * CAR_UART_PORT_1 -> UART_1，PB6  TX / PB7  RX，常用于外接通信模块。
 * CAR_UART_PORT_2 -> UART_2，PB17 TX / PB16 RX，沿用原工程 WIT 串口脚位。
 */
typedef enum {
    CAR_UART_PORT_0 = 0,
    CAR_UART_PORT_1 = 1,
    CAR_UART_PORT_2 = 2,
    CAR_UART_PORT_COUNT = 3
} CarUartPort_t;

#define CAR_UART_RX_BUFFER_SIZE (128U)

/*
 * 每路 UART 的接收缓存结构。
 * data[]: 具体接收到的数据就存放在这里。
 * head: 中断写入位置。
 * tail: 应用读取位置。
 * overflow_count: 缓冲满导致丢字节的次数。
 */
typedef struct {
    volatile uint8_t data[CAR_UART_RX_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t overflow_count;
} CarUartRxBuffer_t;

void CarUart_Init(void);
void CarUart_SendByte(CarUartPort_t port, uint8_t byte);
void CarUart_SendBuffer(CarUartPort_t port, const uint8_t *data, uint16_t len);
void CarUart_SendString(CarUartPort_t port, const char *str);
uint16_t CarUart_Available(CarUartPort_t port);
uint16_t CarUart_Read(CarUartPort_t port, uint8_t *out, uint16_t max_len);
bool CarUart_ReadLine(CarUartPort_t port, char *line, uint16_t max_len);
void CarUart_ClearRx(CarUartPort_t port);
const volatile CarUartRxBuffer_t *CarUart_GetRxBuffer(CarUartPort_t port);

#endif /* UART_COMM_H_ */
