#ifndef MOTOR_SET_SPEED_h
#define MOTOR_SET_SPEED_h

#include "ti_msp_dl_config.h"
#include "motor_crc.h"


typedef struct
{
    float kp;          // 比例系数
    float ki;          // 积分系数
    float kd;          // 微分系数
} PID_t;


void Motor_Set_ClosedLoop(void);//设置电机进入闭环

void Motor_Set_Speeds(int16_t v0, int16_t v1, int16_t v2, int16_t v3);//设置电机速度

void Modbus_ParseFrame(uint8_t data);


//编码器极性取反
void Motor_Set_Enc1_A(void);
void Motor_Set_Enc1_B(void);
void Motor_Set_Enc1_C(void);
void Motor_Set_Enc1_D(void);

//设置pid
void Motor_Set_KP_KI_KD(PID_t *Motor1, PID_t *Motor2, PID_t *Motor3, PID_t *Motor4);

#endif


