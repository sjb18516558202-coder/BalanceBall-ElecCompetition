/*********************************************************************************************************************
* 文件名称          zf_device_icm20602
* 功能说明          ICM20602 六轴惯性传感器驱动 适配逐飞 MSPM0G3507 开源库
* 开发环境          MDK 5.37
* 适用平台          MSPM0G3507
* 使用说明          默认硬件 SPI1：SCK B23、MOSI B22、MISO B21、CS B19
********************************************************************************************************************/

#include "zf_common_debug.h"
#include "zf_device_config.h"
#include "zf_driver_delay.h"
#include "zf_driver_gpio.h"
#include "zf_driver_spi.h"

#include "zf_device_icm20602.h"

int16 icm20602_gyro_x = 0, icm20602_gyro_y = 0, icm20602_gyro_z = 0;             // 三轴陀螺仪原始数据
int16 icm20602_acc_x  = 0, icm20602_acc_y  = 0, icm20602_acc_z  = 0;             // 三轴加速度计原始数据
int16 icm20602_temp_raw = 0;                                                     // 温度原始数据

float icm20602_acc_transition_factor[4]  = {16384.0f, 8192.0f, 4096.0f, 2048.0f};
float icm20602_gyro_transition_factor[4] = {131.0f, 65.5f, 32.8f, 16.4f};

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     ICM20602 写寄存器
// 参数说明     reg             寄存器地址
// 参数说明     data            数据
// 返回参数     void
// 备注信息     内部调用
//-------------------------------------------------------------------------------------------------------------------
static void icm20602_write_register (uint8 reg, uint8 data)
{
    ICM20602_CS(0);
    spi_write_8bit_register(ICM20602_SPI, reg | ICM20602_SPI_W, data);
    ICM20602_CS(1);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     ICM20602 读寄存器
// 参数说明     reg             寄存器地址
// 返回参数     uint8           读取到的数据
// 备注信息     内部调用
//-------------------------------------------------------------------------------------------------------------------
static uint8 icm20602_read_register (uint8 reg)
{
    uint8 data = 0;

    ICM20602_CS(0);
    data = spi_read_8bit_register(ICM20602_SPI, reg | ICM20602_SPI_R);
    ICM20602_CS(1);

    return data;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     ICM20602 连续读寄存器
// 参数说明     reg             起始寄存器地址
// 参数说明     data            数据缓冲区
// 参数说明     len             数据长度
// 返回参数     void
// 备注信息     内部调用，ICM20602 输出数据为高字节在前
//-------------------------------------------------------------------------------------------------------------------
static void icm20602_read_registers (uint8 reg, uint8 *data, uint32 len)
{
    ICM20602_CS(0);
    spi_read_8bit_registers(ICM20602_SPI, reg | ICM20602_SPI_R, data, len);
    ICM20602_CS(1);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     ICM20602 检测 ID
// 参数说明     void
// 返回参数     uint8           1-自检失败 0-自检成功
// 备注信息     ICM20602 WHO_AM_I 默认值为 0x12
//-------------------------------------------------------------------------------------------------------------------
static uint8 icm20602_self_check (void)
{
    uint8 dat = 0;
    uint8 return_state = 0;
    uint16 timeout_count = 0;

    do
    {
        if(ICM20602_TIMEOUT_COUNT < timeout_count ++)
        {
            return_state = 1;
            break;
        }

        dat = icm20602_read_register(ICM20602_REG_WHO_AM_I);
        system_delay_ms(10);
    }while(ICM20602_WHO_AM_I_VALUE != dat);

    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     获取 ICM20602 加速度计数据
// 参数说明     void
// 返回参数     void
// 使用示例     icm20602_get_acc(); 执行后查看 icm20602_acc_x/y/z
//-------------------------------------------------------------------------------------------------------------------
void icm20602_get_acc (void)
{
    uint8 dat[6];

    icm20602_read_registers(ICM20602_REG_ACCEL_XOUT_H, dat, 6);
    icm20602_acc_x = (int16)(((uint16)dat[0] << 8) | dat[1]);
    icm20602_acc_y = (int16)(((uint16)dat[2] << 8) | dat[3]);
    icm20602_acc_z = (int16)(((uint16)dat[4] << 8) | dat[5]);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     获取 ICM20602 陀螺仪数据
// 参数说明     void
// 返回参数     void
// 使用示例     icm20602_get_gyro(); 执行后查看 icm20602_gyro_x/y/z
//-------------------------------------------------------------------------------------------------------------------
void icm20602_get_gyro (void)
{
    uint8 dat[6];

    icm20602_read_registers(ICM20602_REG_GYRO_XOUT_H, dat, 6);
    icm20602_gyro_x = (int16)(((uint16)dat[0] << 8) | dat[1]);
    icm20602_gyro_y = (int16)(((uint16)dat[2] << 8) | dat[3]);
    icm20602_gyro_z = (int16)(((uint16)dat[4] << 8) | dat[5]);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     一次性获取 ICM20602 加速度计、温度、陀螺仪数据
// 参数说明     void
// 返回参数     void
// 使用示例     icm20602_get_acc_gyro(); 执行后查看 icm20602_acc_x/y/z 与 icm20602_gyro_x/y/z
//-------------------------------------------------------------------------------------------------------------------
void icm20602_get_acc_gyro (void)
{
    uint8 dat[14];

    icm20602_read_registers(ICM20602_REG_ACCEL_XOUT_H, dat, 14);
    icm20602_acc_x     = (int16)(((uint16)dat[0]  << 8) | dat[1]);
    icm20602_acc_y     = (int16)(((uint16)dat[2]  << 8) | dat[3]);
    icm20602_acc_z     = (int16)(((uint16)dat[4]  << 8) | dat[5]);
    icm20602_temp_raw  = (int16)(((uint16)dat[6]  << 8) | dat[7]);
    icm20602_gyro_x    = (int16)(((uint16)dat[8]  << 8) | dat[9]);
    icm20602_gyro_y    = (int16)(((uint16)dat[10] << 8) | dat[11]);
    icm20602_gyro_z    = (int16)(((uint16)dat[12] << 8) | dat[13]);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     获取 ICM20602 温度原始数据
// 参数说明     void
// 返回参数     void
// 使用示例     icm20602_get_temp(); 执行后查看 icm20602_temp_raw
//-------------------------------------------------------------------------------------------------------------------
void icm20602_get_temp (void)
{
    uint8 dat[2];

    icm20602_read_registers(ICM20602_REG_TEMP_OUT_H, dat, 2);
    icm20602_temp_raw = (int16)(((uint16)dat[0] << 8) | dat[1]);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     初始化 ICM20602
// 参数说明     void
// 返回参数     uint8           1-初始化失败 0-初始化成功
// 使用示例     icm20602_init();
// 备注信息     默认配置：加速度 ±4g，陀螺仪 ±1000dps，200Hz采样，开启硬件低通，关闭 I2C 接口，使用 SPI 通信
//-------------------------------------------------------------------------------------------------------------------
uint8 icm20602_init (void)
{
    uint8 return_state = 0;

    system_delay_ms(50);                                                         // 等待模块上电稳定

    spi_init(ICM20602_SPI, SPI_MODE0, ICM20602_SPI_SPEED, ICM20602_SPC_PIN, ICM20602_SDI_PIN, ICM20602_SDO_PIN, SPI_CS_NULL);
    gpio_init(ICM20602_CS_PIN, GPO, GPIO_HIGH, GPO_PUSH_PULL);
    system_delay_ms(10);

    do
    {
        // 先读 ID，确认 SPI 通信正常
        if(icm20602_self_check())
        {
            zf_log(0, "ICM20602 self check error, please check SPI wiring and CS pin.");
            return_state = 1;
            break;
        }

        // 复位芯片
        icm20602_write_register(ICM20602_REG_PWR_MGMT_1, 0x80);
        system_delay_ms(100);

        // 复位后再次确认 ID
        if(icm20602_self_check())
        {
            zf_log(0, "ICM20602 reset check error.");
            return_state = 1;
            break;
        }

        // 关闭 I2C 接口，使用 SPI；唤醒芯片并启用六轴
        icm20602_write_register(ICM20602_REG_I2C_IF,     0x40);                  // I2C_IF_DIS = 1
        icm20602_write_register(ICM20602_REG_PWR_MGMT_1, 0x01);                  // 选择 PLL 时钟并退出睡眠
        icm20602_write_register(ICM20602_REG_PWR_MGMT_2, 0x00);                  // 使能加速度计和陀螺仪所有轴
        system_delay_ms(10);

        icm20602_write_register(ICM20602_REG_FIFO_EN,    0x00);                  // 不使用 FIFO
        icm20602_write_register(ICM20602_REG_INT_ENABLE, 0x00);                  // 不使用中断
        icm20602_write_register(ICM20602_REG_SMPLRT_DIV, 0x04);                  // 1kHz/(1+4)=200Hz，和上层滤波周期匹配
        icm20602_write_register(ICM20602_REG_CONFIG,     0x03);                  // 陀螺仪硬件 DLPF 约 41Hz，兼顾小车抗震与转向响应
        icm20602_write_register(ICM20602_REG_ACCEL_CONFIG2, 0x04);               // 加速度计硬件 DLPF 约 20Hz，降低电机/底盘震动影响

        switch(ICM20602_ACC_SAMPLE_DEFAULT)
        {
            default:
            {
                zf_log(0, "ICM20602_ACC_SAMPLE_DEFAULT set error.");
                return_state = 1;
            }break;
            case ICM20602_ACC_SAMPLE_SGN_2G:     icm20602_write_register(ICM20602_REG_ACCEL_CONFIG, 0x00);   break;
            case ICM20602_ACC_SAMPLE_SGN_4G:     icm20602_write_register(ICM20602_REG_ACCEL_CONFIG, 0x08);   break;
            case ICM20602_ACC_SAMPLE_SGN_8G:     icm20602_write_register(ICM20602_REG_ACCEL_CONFIG, 0x10);   break;
            case ICM20602_ACC_SAMPLE_SGN_16G:    icm20602_write_register(ICM20602_REG_ACCEL_CONFIG, 0x18);   break;
        }
        if(1 == return_state)
        {
            break;
        }

        switch(ICM20602_GYRO_SAMPLE_DEFAULT)
        {
            default:
            {
                zf_log(0, "ICM20602_GYRO_SAMPLE_DEFAULT set error.");
                return_state = 1;
            }break;
            case ICM20602_GYRO_SAMPLE_SGN_250DPS:    icm20602_write_register(ICM20602_REG_GYRO_CONFIG, 0x00); break;
            case ICM20602_GYRO_SAMPLE_SGN_500DPS:    icm20602_write_register(ICM20602_REG_GYRO_CONFIG, 0x08); break;
            case ICM20602_GYRO_SAMPLE_SGN_1000DPS:   icm20602_write_register(ICM20602_REG_GYRO_CONFIG, 0x10); break;
            case ICM20602_GYRO_SAMPLE_SGN_2000DPS:   icm20602_write_register(ICM20602_REG_GYRO_CONFIG, 0x18); break;
        }
    }while(0);

    return return_state;
}
