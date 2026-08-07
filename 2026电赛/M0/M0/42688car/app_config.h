#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#include <stdint.h>

/*
 * app_config.h 是整车参数集中管理文件。
 * 以后调车时，优先来这里改参数，不要分散到各个 .c 文件里找。
 */

/* 控制周期：CONTROL_TIMER 每 10ms 进一次中断，速度单位也是“每 10ms 的编码器 tick”。 */
#define CAR_CONTROL_PERIOD_MS                 (10U)
#define CAR_CONTROL_PERIOD_S                  (0.01f)

/* PWM 计数最大值。SysConfig 中 PWM_MOTOR.timerCount 也是 1600，两边必须一致。 */
#define CAR_PWM_MAX                           (1600)
#define CAR_PWM_DEAD_BAND                     (0)

/*
 * 速度环 PID 参数，使用你原工程里的赋值：
 * SPEED_KP = 50
 * SPEED_KI = 18
 *
 * 当前速度环函数 SpeedPID_Update() 是增量式 PI：
 * output += Kp * (本次误差 - 上次误差) + Ki * 本次误差。
 * 原工程速度环没有使用 Kd，所以这里 CAR_SPEED_KD 先给 0。
 */
#define CAR_SPEED_KP                          (50.0f)
#define CAR_SPEED_KI                          (18.0f)
#define CAR_SPEED_KD                          (0.0f)

/*
 * 角度环 PID 参数，使用你原工程 SpeedPIDJD_Init() 里的赋值：
 * Kp = 0.39
 * Ki = 0.01
 * Kd = 0.03
 *
 * 角度环误差单位是“度”。
 * Kd 使用 ICM42688 陀螺仪 Z 轴角速度 gyro_z_dps，
 * 小车转动越快，-Kd * gyro_z_dps 对输出的抑制越明显。
 */
#define CAR_ANGLE_KP                          (0.39f)
#define CAR_ANGLE_KI                          (0.01f)
#define CAR_ANGLE_KD                          (0.03f)
#define CAR_ANGLE_OUTPUT_LIMIT                (40.0f)
#define CAR_ANGLE_INTEGRAL_LIMIT              (100.0f)

/* 里程计参数：需要按真实车轮直径、轮距和编码器线数标定。 */
#define CAR_WHEEL_DIAMETER_M                  (0.065f)
#define CAR_WHEEL_TRACK_M                     (0.145f)
#define CAR_ENCODER_TICKS_PER_REV             (1560.0f)

/* 右轮只统计 A 相上升沿，所以乘 4 近似换算到四倍频 QEI tick。 */
#define CAR_RIGHT_ENCODER_EDGE_SCALE          (4)

/* 如果实车转动方向与软件正方向相反，把对应宏从 0 改为 1。 */
#define CAR_LEFT_ENCODER_DIR_INVERT           (0)
#define CAR_RIGHT_ENCODER_DIR_INVERT          (0)

/* 如果发现 TIMG6 计数方向是反的，把这里改成 1。 */
#define CAR_RIGHT_EDGE_TIMER_COUNTS_DOWN      (0)

/* ICM42688 量程换算参数：当前初始化为 +-16g、+-2000dps。 */
#define ICM42688_WHO_AM_I_EXPECTED            (0x47U)
#define ICM42688_ACCEL_LSB_PER_G              (2048.0f)
#define ICM42688_GYRO_LSB_PER_DPS             (16.4f)
#define ICM42688_TEMP_LSB_PER_C               (132.48f)
#define ICM42688_TEMP_OFFSET_C                (25.0f)

#endif /* APP_CONFIG_H_ */
