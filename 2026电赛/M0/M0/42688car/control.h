#ifndef CONTROL_H_
#define CONTROL_H_

#include <stdbool.h>
#include <stdint.h>

#include "odometry.h"

/*
 * CarControlState_t 是控制层对外公开的状态快照。
 * OLED 显示、串口调试或上位机读取状态时，不需要直接访问 PID 内部变量。
 */
typedef struct {
    /* 用户命令给定的左右轮目标速度，单位：每 10ms 编码器 tick。 */
    int16_t target_left_speed;
    int16_t target_right_speed;

    /* Encoder_Update() 本周期测得的左右轮速度，单位同目标速度。 */
    int32_t measured_left_speed;
    int32_t measured_right_speed;

    /* 速度 PID 最终输出到 Motor_SetPwm() 的 PWM 值，范围约为 +/-1600。 */
    int16_t left_pwm;
    int16_t right_pwm;

    /* 角度环目标角度、当前 IMU 积分角度以及角度环修正量。 */
    float target_angle_deg;
    float current_angle_deg;
    float angle_correction;

    /* true 时启用角度环，false 时只做左右轮速度闭环。 */
    bool angle_loop_enable;

    /* ICM42688 是否初始化并读数成功，显示层会用它提示 IMU 状态。 */
    bool imu_ready;
} CarControlState_t;

/* 初始化电机、编码器、PID 和里程计。 */
void Car_ControlInit(void);

/*
 * 小车统一控制入口。
 * left_speed/right_speed: 左右轮目标速度。
 * angle_loop_enable: 是否启用角度环。
 * target_angle_deg: 启用角度环时的目标航向角，单位为度。
 */
void Car_MotorControl(int16_t left_speed, int16_t right_speed,
                      bool angle_loop_enable, float target_angle_deg);

/* 10ms 定时器中断里调用的控制更新函数。 */
void Car_ControlUpdate10ms(void);

/* 立即停车并复位 PID 输出。 */
void Car_Stop(void);

/* 获取一份控制状态快照，供 OLED 或串口调试使用。 */
CarControlState_t Car_GetControlState(void);

#endif /* CONTROL_H_ */
