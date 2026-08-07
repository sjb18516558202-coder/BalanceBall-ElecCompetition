/*********************************************************************************************************************
* 文件名称          zf_device_icm20602
* 功能说明          ICM20602 六轴惯性传感器驱动 适配逐飞 MSPM0G3507 开源库
* 说明              由通用 ICM20602 SPI 寄存器读写逻辑移植而来
********************************************************************************************************************/

#ifndef _zf_device_icm20602_h_
#define _zf_device_icm20602_h_

#include "zf_common_typedef.h"

//====================================================硬件 SPI 驱动====================================================
// 默认沿用 E4_imu 例程中 IMU660RB 的 SPI1 接线，若你的 ICM20602 接线不同，只需要修改下面几个宏
#define ICM20602_SPI_SPEED                         ( 8 * 1000 * 1000 )          // ICM20602 SPI 速率，常用 1MHz~8MHz
#define ICM20602_SPI                               ( SPI_1         )            // 硬件 SPI 号
#define ICM20602_SPC_PIN                           ( SPI1_SCK_B23  )            // SCLK / SPC
#define ICM20602_SDI_PIN                           ( SPI1_MOSI_B22 )            // MOSI / SDI
#define ICM20602_SDO_PIN                           ( SPI1_MISO_B21 )            // MISO / SDO
#define ICM20602_CS_PIN                            ( B19 )                      // CS 片选引脚
#define ICM20602_CS(x)                             (x ? (gpio_high(ICM20602_CS_PIN)) : (gpio_low(ICM20602_CS_PIN)))
//====================================================硬件 SPI 驱动====================================================

#define ICM20602_SPI_W                             ( 0x00 )
#define ICM20602_SPI_R                             ( 0x80 )
#define ICM20602_TIMEOUT_COUNT                     ( 100 )

// ICM20602 常用寄存器
#define ICM20602_REG_SMPLRT_DIV                    ( 0x19 )
#define ICM20602_REG_CONFIG                        ( 0x1A )
#define ICM20602_REG_GYRO_CONFIG                   ( 0x1B )
#define ICM20602_REG_ACCEL_CONFIG                  ( 0x1C )
#define ICM20602_REG_ACCEL_CONFIG2                 ( 0x1D )
#define ICM20602_REG_FIFO_EN                       ( 0x23 )
#define ICM20602_REG_INT_ENABLE                    ( 0x38 )
#define ICM20602_REG_ACCEL_XOUT_H                  ( 0x3B )
#define ICM20602_REG_TEMP_OUT_H                    ( 0x41 )
#define ICM20602_REG_GYRO_XOUT_H                   ( 0x43 )
#define ICM20602_REG_USER_CTRL                     ( 0x6A )
#define ICM20602_REG_PWR_MGMT_1                    ( 0x6B )
#define ICM20602_REG_PWR_MGMT_2                    ( 0x6C )
#define ICM20602_REG_I2C_IF                        ( 0x70 )
#define ICM20602_REG_WHO_AM_I                      ( 0x75 )
#define ICM20602_WHO_AM_I_VALUE                    ( 0x12 )

typedef enum
{
    ICM20602_ACC_SAMPLE_SGN_2G = 0,                                              // 加速度计量程 ±2g
    ICM20602_ACC_SAMPLE_SGN_4G,                                                  // 加速度计量程 ±4g
    ICM20602_ACC_SAMPLE_SGN_8G,                                                  // 加速度计量程 ±8g
    ICM20602_ACC_SAMPLE_SGN_16G,                                                 // 加速度计量程 ±16g
}icm20602_acc_sample_config;

typedef enum
{
    ICM20602_GYRO_SAMPLE_SGN_250DPS = 0,                                         // 陀螺仪量程 ±250dps
    ICM20602_GYRO_SAMPLE_SGN_500DPS,                                             // 陀螺仪量程 ±500dps
    ICM20602_GYRO_SAMPLE_SGN_1000DPS,                                            // 陀螺仪量程 ±1000dps
    ICM20602_GYRO_SAMPLE_SGN_2000DPS,                                            // 陀螺仪量程 ±2000dps
}icm20602_gyro_sample_config;

// 默认配置：与之前 ICM20602 程序保持一致：加速度 ±4g，陀螺仪 ±250dps
#define ICM20602_ACC_SAMPLE_DEFAULT                 ( ICM20602_ACC_SAMPLE_SGN_4G )
#define ICM20602_GYRO_SAMPLE_DEFAULT                ( ICM20602_GYRO_SAMPLE_SGN_250DPS )

extern int16 icm20602_gyro_x;
extern int16 icm20602_gyro_y;
extern int16 icm20602_gyro_z;
extern int16 icm20602_acc_x;
extern int16 icm20602_acc_y;
extern int16 icm20602_acc_z;
extern int16 icm20602_temp_raw;

extern float icm20602_acc_transition_factor[4];
extern float icm20602_gyro_transition_factor[4];

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     将 ICM20602 加速度计原始数据转换为 g
// 参数说明     acc_value       任意轴的加速度计原始数据
// 返回参数     float           单位 g
//-------------------------------------------------------------------------------------------------------------------
#define icm20602_acc_transition(acc_value)          ((float)(acc_value) / icm20602_acc_transition_factor[ICM20602_ACC_SAMPLE_DEFAULT])

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     将 ICM20602 陀螺仪原始数据转换为 °/s
// 参数说明     gyro_value      任意轴的陀螺仪原始数据
// 返回参数     float           单位 °/s
//-------------------------------------------------------------------------------------------------------------------
#define icm20602_gyro_transition(gyro_value)        ((float)(gyro_value) / icm20602_gyro_transition_factor[ICM20602_GYRO_SAMPLE_DEFAULT])

void    icm20602_get_acc                 (void);
void    icm20602_get_gyro                (void);
void    icm20602_get_acc_gyro            (void);
void    icm20602_get_temp                (void);
uint8   icm20602_init                    (void);

#endif
