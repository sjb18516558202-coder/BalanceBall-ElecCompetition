#ifndef MOTOR_H
#define MOTOR_H

#include "zf_common_headfile.h"
#include "motorsetspeed.h"

extern volatile int16_t encoder_values[4];
extern volatile int16_t encoder_speeds[4];
extern volatile int16_t modbus_date[8];
extern volatile uint8_t modbus_rx_frame_done;

void Motor_init(void);
uint8_t Motor_Read_Encoders(void);
void Modbus_ParseFrame(uint8_t data);

#endif
