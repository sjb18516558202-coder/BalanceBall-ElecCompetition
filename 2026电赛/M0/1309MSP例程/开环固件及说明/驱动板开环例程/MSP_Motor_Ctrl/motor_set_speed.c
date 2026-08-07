#include "motor_set_speed.h"




void set_pwm_duty(int16_t v1, int16_t v2, int16_t v3, int16_t v4)
{
    uint8_t idx = 0;
		uint8_t frame[20];

    frame[idx++] = 0x0A;        // 从站地址
    frame[idx++] = 0x03;        // 功能码：写多个保持寄存器

    frame[idx++] = 0x10;        // 起始寄存器高字节
   

    frame[idx++] = (v1 >> 8) & 0xFF;
    frame[idx++] = (v1 >> 0) & 0xFF;


    frame[idx++] = (v2 >> 8) & 0xFF;
    frame[idx++] = (v2 >> 0) & 0xFF;

    frame[idx++] = (v3 >> 8) & 0xFF;
    frame[idx++] = (v3 >> 0) & 0xFF;

    frame[idx++] = (v4 >> 8) & 0xFF;
    frame[idx++] = (v4 >> 0) & 0xFF;

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;

    // 发送
    for (uint8_t i = 0; i < idx; i++)
    {
        while (DL_UART_isBusy(MSPMotor_INST));
        DL_UART_Main_transmitData(MSPMotor_INST, frame[i]);
    }
}
