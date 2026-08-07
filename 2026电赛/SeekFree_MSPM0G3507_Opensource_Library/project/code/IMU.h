/*********************************************************************************************************************
* MSPM0G3507 Opensource Library 即（MSPM0G3507 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 文件名称          IMU
* 说明              IMU 陀螺仪模块驱动（UART 通信）
*                   修改：添加零偏校准、改进积分、添加稳定检测
********************************************************************************************************************/

#ifndef IMU_h
#define IMU_h

#include "zf_common_headfile.h"
#include "motorcrc.h"

extern volatile float imu_angle;            // 当前角度 (度)
extern volatile float imu_dps;              // 当前角速度 (度/秒)，已扣除零偏
extern volatile float imu_dps_raw_nobias;   // 原始角速度 (度/秒)，未扣除零偏
extern volatile float imu_yaw_sum;          // 角速度积分得到的累计偏航角，单位度
extern volatile float imu_bias_dps;         // 零偏校准值 (度/秒)
extern volatile uint8_t imu_calibrated;     // 校准完成标志：0=校准中，1=已校准
extern volatile uint32_t imu_rx_byte_count; // UART3 收到的原始字节数
extern volatile uint32_t imu_rx_frame_count;// CRC 正确的有效帧数量
extern volatile uint32_t imu_rx_crc_error;  // CRC 错误帧数量

void IMU_Init(void);
uint8_t IMU_Read(void);
void IMU_UART_Handler(void);
void IMU_UART_ReceiveByte(uint8_t data);

/* 新加：零偏自动校准（阻塞式，需要小车静止） */
void IMU_BiasCalibrate(uint16_t sample_time_ms);

/* 新加：重置累计偏航角 */
void IMU_ResetYaw(void);

#endif
