#include "motor.h"
#include "motorcrc.h"

volatile int16_t encoder_values[4] = {0, 0, 0, 0};
volatile int16_t encoder_speeds[4] = {0, 0, 0, 0};

volatile int16_t modbus_date[8];
volatile uint8_t modbus_rx_frame_done;

void Motor_init(void)
{
    // 驱动板闭环初始化：只进入闭环并清零目标速度，PID 使用驱动板固件自带参数。
    Motor_Set_ClosedLoop();
    system_delay_ms(50);
    Motor_Stop_All();
    system_delay_ms(50);

    // 如需覆盖驱动板自带 PID，再取消下面注释。默认注释掉，使用驱动板固件自带参数。
    // PID_t m1 = {1.2f, 0.05f, 0.01f};
    // PID_t m2 = {1.2f, 0.05f, 0.01f};
    // PID_t m3 = {1.2f, 0.05f, 0.01f};
    // PID_t m4 = {1.2f, 0.05f, 0.01f};
    // Motor_Set_KP_KI_KD(&m1, &m2, &m3, &m4);
}

#define ENCODER_REG_BASE_ADDR   0x0032
#define ENCODER_REG_COUNT       8

uint8_t Motor_Read_Encoders(void)
{
    uint8_t idx = 0;
    uint8_t frame[8];
    uint32_t timeout;

    modbus_rx_frame_done = 0;

    // 读取驱动板连续 8 个寄存器：前 4 个为编码器计数，后 4 个为速度反馈。
    frame[idx++] = 0x0A;
    frame[idx++] = 0x03;
    frame[idx++] = (uint8_t)((ENCODER_REG_BASE_ADDR >> 8) & 0xFF);
    frame[idx++] = (uint8_t)(ENCODER_REG_BASE_ADDR & 0xFF);
    frame[idx++] = 0x00;
    frame[idx++] = ENCODER_REG_COUNT;

    {
        uint16_t crc = CRC16(frame, idx);
        frame[idx++] = (uint8_t)(crc & 0xFF);
        frame[idx++] = (uint8_t)((crc >> 8) & 0xFF);
    }

    uart_write_buffer(UART_2, frame, idx);

    timeout = 500000;
    while(!modbus_rx_frame_done && timeout > 0)
    {
        uint8_t byte;
        if(uart_query_byte(UART_2, &byte))
        {
            Modbus_ParseFrame(byte);
        }
        timeout--;
    }

    if(modbus_rx_frame_done)
    {
        encoder_values[0] = modbus_date[0];
        encoder_values[1] = modbus_date[1];
        encoder_values[2] = modbus_date[2];
        encoder_values[3] = modbus_date[3];

        encoder_speeds[0] = modbus_date[4];
        encoder_speeds[1] = modbus_date[5];
        encoder_speeds[2] = modbus_date[6];
        encoder_speeds[3] = modbus_date[7];
        return 1;
    }

    return 0;
}

#define MODBUS_MAX_REG 8

void Modbus_ParseFrame(uint8_t data)
{
    static uint8_t state = 0;
    static uint8_t modbus_reg_num;
    static uint8_t data_idx = 0;
    static uint16_t reg_temp = 0;
    static uint8_t crc_l = 0;
    static uint8_t crc_h = 0;
    static uint8_t modbus_raw_buf[32];
    static uint8_t raw_idx = 0;
    static uint8_t modbus_rx_byte_cnt = 0;
    static uint16_t modbus_reg_buf[MODBUS_MAX_REG];

    if(raw_idx < sizeof(modbus_raw_buf))
    {
        modbus_raw_buf[raw_idx++] = data;
    }
    else
    {
        raw_idx = 0;
    }

    switch(state)
    {
        case 0:
            if(data == 0x0A)
            {
                state = 1;
            }
            else
            {
                raw_idx = 0;
            }
            break;

        case 1:
            if(data == 0x03)
            {
                state = 2;
            }
            else
            {
                state = 0;
                raw_idx = 0;
            }
            break;

        case 2:
            modbus_rx_byte_cnt = data;
            if((modbus_rx_byte_cnt & 0x01) || (modbus_rx_byte_cnt / 2 > MODBUS_MAX_REG))
            {
                state = 0;
                raw_idx = 0;
                break;
            }
            modbus_reg_num = modbus_rx_byte_cnt / 2;
            data_idx = 0;
            state = 3;
            break;

        case 3:
            reg_temp = ((uint16_t)data << 8);
            state = 4;
            break;

        case 4:
            reg_temp |= data;
            if(data_idx < MODBUS_MAX_REG)
            {
                modbus_reg_buf[data_idx++] = reg_temp;
            }
            state = ((data_idx * 2) >= modbus_rx_byte_cnt) ? 5 : 3;
            break;

        case 5:
            crc_l = data;
            state = 6;
            break;

        case 6:
        {
            uint16_t crc_calc;
            uint16_t crc_recv;

            crc_h = data;
            crc_recv = crc_l | ((uint16_t)crc_h << 8);
            crc_calc = CRC16(modbus_raw_buf, 3 + modbus_rx_byte_cnt);

            if(crc_calc == crc_recv)
            {
                uint8_t i;
                modbus_rx_frame_done = 1;
                for(i = 0; i < modbus_reg_num; i++)
                {
                    modbus_date[i] = modbus_reg_buf[i];
                }
            }

            state = 0;
            raw_idx = 0;
            break;
        }

        default:
            state = 0;
            raw_idx = 0;
            break;
    }
}
