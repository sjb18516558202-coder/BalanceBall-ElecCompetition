#include "uart_comm.h"

#include "ti_msp_dl_config.h"

/*
 * UART 接收设计：
 * 1. SysConfig 已经给 UART_0/UART_1/UART_2 打开 RX 中断。
 * 2. 中断函数收到字节后，马上写入 g_uart_rx[port].data[]。
 * 3. g_uart_rx[port].data[] 就是“接收到哪”的具体位置。
 * 4. 应用层使用 CarUart_Read() 读取任意字节流。
 * 5. 应用层使用 CarUart_ReadLine() 读取以 '\n' 结束的一行命令。
 * 6. 缓冲区满时丢弃新字节，并增加 overflow_count，方便调试。
 */

static CarUartRxBuffer_t g_uart_rx[CAR_UART_PORT_COUNT];

static UART_Regs *uart_instance(CarUartPort_t port)
{
    switch (port) {
        case CAR_UART_PORT_0:
            return UART_0_INST;
        case CAR_UART_PORT_1:
            return UART_1_INST;
        case CAR_UART_PORT_2:
            return UART_2_INST;
        default:
            return 0;
    }
}

static bool valid_port(CarUartPort_t port)
{
    return ((uint32_t) port < (uint32_t) CAR_UART_PORT_COUNT);
}

static uint16_t next_index(uint16_t value)
{
    value++;
    if (value >= CAR_UART_RX_BUFFER_SIZE) {
        value = 0U;
    }
    return value;
}

static void rx_push(CarUartPort_t port, uint8_t byte)
{
    CarUartRxBuffer_t *rx;
    uint16_t next_head;

    if (!valid_port(port)) {
        return;
    }

    rx = &g_uart_rx[port];
    next_head = next_index(rx->head);

    if (next_head == rx->tail) {
        rx->overflow_count++;
        return;
    }

    rx->data[rx->head] = byte;
    rx->head = next_head;
}

static void drain_uart_to_buffer(CarUartPort_t port, UART_Regs *uart)
{
    uint8_t byte;

    /* 一次中断可能 FIFO 里已有多个字节，所以循环搬空。 */
    while (DL_UART_Main_receiveDataCheck(uart, &byte)) {
        rx_push(port, byte);
    }
}

static void handle_uart_irq(CarUartPort_t port, UART_Regs *uart)
{
    switch (DL_UART_Main_getPendingInterrupt(uart)) {
        case DL_UART_MAIN_IIDX_RX:
        case DL_UART_MAIN_IIDX_RX_TIMEOUT_ERROR:
            drain_uart_to_buffer(port, uart);
            break;
        case DL_UART_MAIN_IIDX_OVERRUN_ERROR:
        case DL_UART_MAIN_IIDX_BREAK_ERROR:
        case DL_UART_MAIN_IIDX_PARITY_ERROR:
        case DL_UART_MAIN_IIDX_FRAMING_ERROR:
            /* 出错时也读走 FIFO，避免错误字节堵住后续接收。 */
            drain_uart_to_buffer(port, uart);
            break;
        default:
            break;
    }
}

void CarUart_Init(void)
{
    CarUart_ClearRx(CAR_UART_PORT_0);
    CarUart_ClearRx(CAR_UART_PORT_1);
    CarUart_ClearRx(CAR_UART_PORT_2);

    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(UART_1_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(UART_2_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_1_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_2_INST_INT_IRQN);
}

void CarUart_SendByte(CarUartPort_t port, uint8_t byte)
{
    UART_Regs *uart = uart_instance(port);

    if (uart == 0) {
        return;
    }

    DL_UART_Main_transmitDataBlocking(uart, byte);
}

void CarUart_SendBuffer(CarUartPort_t port, const uint8_t *data, uint16_t len)
{
    uint16_t i;

    if (data == 0) {
        return;
    }

    for (i = 0U; i < len; i++) {
        CarUart_SendByte(port, data[i]);
    }
}

void CarUart_SendString(CarUartPort_t port, const char *str)
{
    if (str == 0) {
        return;
    }

    while (*str != '\0') {
        CarUart_SendByte(port, (uint8_t) *str);
        str++;
    }
}

uint16_t CarUart_Available(CarUartPort_t port)
{
    const CarUartRxBuffer_t *rx;
    uint16_t head;
    uint16_t tail;

    if (!valid_port(port)) {
        return 0U;
    }

    rx = &g_uart_rx[port];
    head = rx->head;
    tail = rx->tail;

    if (head >= tail) {
        return (uint16_t) (head - tail);
    }
    return (uint16_t) (CAR_UART_RX_BUFFER_SIZE - tail + head);
}

uint16_t CarUart_Read(CarUartPort_t port, uint8_t *out, uint16_t max_len)
{
    CarUartRxBuffer_t *rx;
    uint16_t count = 0U;

    if (!valid_port(port) || (out == 0)) {
        return 0U;
    }

    rx = &g_uart_rx[port];
    while ((rx->tail != rx->head) && (count < max_len)) {
        out[count++] = rx->data[rx->tail];
        rx->tail = next_index(rx->tail);
    }

    return count;
}

bool CarUart_ReadLine(CarUartPort_t port, char *line, uint16_t max_len)
{
    CarUartRxBuffer_t *rx;
    uint16_t local_tail;
    uint16_t length = 0U;
    bool found_newline = false;

    if (!valid_port(port) || (line == 0) || (max_len < 2U)) {
        return false;
    }

    rx = &g_uart_rx[port];
    local_tail = rx->tail;

    /* 没有收到完整换行前，不移动 tail，半包会继续留在缓冲区。 */
    while (local_tail != rx->head) {
        char ch = (char) rx->data[local_tail];
        local_tail = next_index(local_tail);
        if (ch == '\n') {
            found_newline = true;
            break;
        }
    }

    if (!found_newline) {
        return false;
    }

    while (rx->tail != rx->head) {
        char ch = (char) rx->data[rx->tail];
        rx->tail = next_index(rx->tail);

        if ((ch == '\n') || (ch == '\r')) {
            if (ch == '\n') {
                break;
            }
            continue;
        }

        if (length < (uint16_t) (max_len - 1U)) {
            line[length++] = ch;
        }
    }
    line[length] = '\0';

    return true;
}

void CarUart_ClearRx(CarUartPort_t port)
{
    CarUartRxBuffer_t *rx;

    if (!valid_port(port)) {
        return;
    }

    rx = &g_uart_rx[port];
    rx->head = 0U;
    rx->tail = 0U;
    rx->overflow_count = 0U;
}

const volatile CarUartRxBuffer_t *CarUart_GetRxBuffer(CarUartPort_t port)
{
    if (!valid_port(port)) {
        return 0;
    }
    return &g_uart_rx[port];
}

void UART_0_INST_IRQHandler(void)
{
    handle_uart_irq(CAR_UART_PORT_0, UART_0_INST);
}

void UART_1_INST_IRQHandler(void)
{
    handle_uart_irq(CAR_UART_PORT_1, UART_1_INST);
}

void UART_2_INST_IRQHandler(void)
{
    handle_uart_irq(CAR_UART_PORT_2, UART_2_INST);
}
