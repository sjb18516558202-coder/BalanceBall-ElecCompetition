#ifndef ICM42688_H_
#define ICM42688_H_

#include <stdbool.h>
#include <stdint.h>

/*
 * ICM42688_Data_t 保存 IMU 的原始数据和换算后的物理量。
 * 当前工程使用 SPI 模式读取 ICM-42688，yaw_deg 由 Z 轴陀螺仪积分得到。
 */
typedef struct {
    /* 原始加速度、陀螺仪和温度 ADC 值。 */
    int16_t raw_accel[3];
    int16_t raw_gyro[3];
    int16_t raw_temp;

    /* 换算后的加速度 g、角速度 dps 和温度摄氏度。 */
    float accel_g[3];
    float gyro_dps[3];
    float temperature_c;

    /* Z 轴角速度积分得到的航向角，单位：度。 */
    float yaw_deg;

    /* WHO_AM_I 寄存器读数，ICM42688 期望值为 0x47。 */
    uint8_t who_am_i;

    /* true 表示芯片识别成功且可用于角度环。 */
    bool present;
} ICM42688_Data_t;

/* 初始化 ICM42688 SPI 通信和量程配置。 */
void ICM42688_Init(void);

/* 查询 IMU 是否识别成功。 */
bool ICM42688_IsReady(void);

/* 读取一次 IMU 数据，成功时也会更新模块内部缓存。 */
bool ICM42688_ReadData(ICM42688_Data_t *out);

/* 获取最近一次 IMU 数据缓存指针。 */
const ICM42688_Data_t *ICM42688_GetData(void);

/* 手动设置当前 yaw 角，常用于上电后清零航向。 */
void ICM42688_SetYaw(float yaw_deg);

#endif /* ICM42688_H_ */
