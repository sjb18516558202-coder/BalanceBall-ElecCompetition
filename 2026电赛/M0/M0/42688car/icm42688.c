#include "icm42688.h"

#include "app_config.h"
#include "clock.h"
#include "pid.h"
#include "ti_msp_dl_config.h"

#define ICM42688_REG_DEVICE_CONFIG      (0x11U)
#define ICM42688_REG_TEMP_DATA1         (0x1DU)
#define ICM42688_REG_PWR_MGMT0          (0x4EU)
#define ICM42688_REG_GYRO_CONFIG0       (0x4FU)
#define ICM42688_REG_ACCEL_CONFIG0      (0x50U)
#define ICM42688_REG_WHO_AM_I           (0x75U)

#define ICM42688_SPI_READ_BIT           (0x80U)
#define ICM42688_SOFT_RESET             (0x01U)
#define ICM42688_PWR_ACCEL_GYRO_LN      (0x0FU)
#define ICM42688_1KHZ_16G_2000DPS       (0x06U)

static ICM42688_Data_t g_imu;

static void imu_cs_low(void)
{
    DL_GPIO_clearPins(GPIO_IMU_PORT, GPIO_IMU_PIN_IMU_CS_PIN);
}

static void imu_cs_high(void)
{
    while (DL_SPI_isBusy(SPI_IMU_INST)) {
    }
    DL_GPIO_setPins(GPIO_IMU_PORT, GPIO_IMU_PIN_IMU_CS_PIN);
}

static uint8_t spi_transfer(uint8_t data)
{
    /* SPI 全双工：发送 1 字节的同时会接收 1 字节。 */
    DL_SPI_transmitDataBlocking8(SPI_IMU_INST, data);
    return DL_SPI_receiveDataBlocking8(SPI_IMU_INST);
}

static uint8_t read_reg(uint8_t reg)
{
    uint8_t value;

    /* 读寄存器：地址最高位置 1，然后再发一个 dummy 字节读回数据。 */
    imu_cs_low();
    (void) spi_transfer(reg | ICM42688_SPI_READ_BIT);
    value = spi_transfer(0x00U);
    imu_cs_high();

    return value;
}

static void write_reg(uint8_t reg, uint8_t value)
{
    /* 写寄存器：地址最高位保持 0，随后发送要写入的值。 */
    imu_cs_low();
    (void) spi_transfer(reg & (uint8_t) ~ICM42688_SPI_READ_BIT);
    (void) spi_transfer(value);
    imu_cs_high();
}

static void read_regs(uint8_t start_reg, uint8_t *buffer, uint8_t len)
{
    uint8_t i;

    imu_cs_low();
    (void) spi_transfer(start_reg | ICM42688_SPI_READ_BIT);
    for (i = 0; i < len; i++) {
        buffer[i] = spi_transfer(0x00U);
    }
    imu_cs_high();
}

static int16_t be_i16(uint8_t msb, uint8_t lsb)
{
    return (int16_t) (((uint16_t) msb << 8) | lsb);
}

void ICM42688_Init(void)
{
    /*
     * ICM42688 初始化顺序：
     * 1. CS 拉高，等待芯片上电稳定。
     * 2. 软件复位。
     * 3. 读取 WHO_AM_I，确认器件存在。
     * 4. 设置陀螺仪/加速度量程和输出速率。
     * 5. 打开加速度计和陀螺仪低噪声模式。
     */
    DL_GPIO_setPins(GPIO_IMU_PORT, GPIO_IMU_PIN_IMU_CS_PIN);
    mspm0_delay_ms(20);

    write_reg(ICM42688_REG_DEVICE_CONFIG, ICM42688_SOFT_RESET);
    mspm0_delay_ms(50);

    g_imu.who_am_i = read_reg(ICM42688_REG_WHO_AM_I);
    g_imu.present = (g_imu.who_am_i == ICM42688_WHO_AM_I_EXPECTED);

    if (!g_imu.present) {
        return;
    }

    write_reg(ICM42688_REG_GYRO_CONFIG0, ICM42688_1KHZ_16G_2000DPS);
    write_reg(ICM42688_REG_ACCEL_CONFIG0, ICM42688_1KHZ_16G_2000DPS);
    write_reg(ICM42688_REG_PWR_MGMT0, ICM42688_PWR_ACCEL_GYRO_LN);
    mspm0_delay_ms(100);
}

bool ICM42688_IsReady(void)
{
    return g_imu.present;
}

bool ICM42688_ReadData(ICM42688_Data_t *out)
{
    uint8_t raw[14];

    if (!g_imu.present) {
        if (out != 0) {
            *out = g_imu;
        }
        return false;
    }

    /* 从 TEMP_DATA1 开始连续读取 14 字节：温度、加速度 XYZ、陀螺仪 XYZ。 */
    read_regs(ICM42688_REG_TEMP_DATA1, raw, sizeof(raw));

    g_imu.raw_temp = be_i16(raw[0], raw[1]);
    g_imu.raw_accel[0] = be_i16(raw[2], raw[3]);
    g_imu.raw_accel[1] = be_i16(raw[4], raw[5]);
    g_imu.raw_accel[2] = be_i16(raw[6], raw[7]);
    g_imu.raw_gyro[0] = be_i16(raw[8], raw[9]);
    g_imu.raw_gyro[1] = be_i16(raw[10], raw[11]);
    g_imu.raw_gyro[2] = be_i16(raw[12], raw[13]);

    g_imu.temperature_c = ((float) g_imu.raw_temp / ICM42688_TEMP_LSB_PER_C) +
                          ICM42688_TEMP_OFFSET_C;
    g_imu.accel_g[0] = (float) g_imu.raw_accel[0] / ICM42688_ACCEL_LSB_PER_G;
    g_imu.accel_g[1] = (float) g_imu.raw_accel[1] / ICM42688_ACCEL_LSB_PER_G;
    g_imu.accel_g[2] = (float) g_imu.raw_accel[2] / ICM42688_ACCEL_LSB_PER_G;
    g_imu.gyro_dps[0] = (float) g_imu.raw_gyro[0] / ICM42688_GYRO_LSB_PER_DPS;
    g_imu.gyro_dps[1] = (float) g_imu.raw_gyro[1] / ICM42688_GYRO_LSB_PER_DPS;
    g_imu.gyro_dps[2] = (float) g_imu.raw_gyro[2] / ICM42688_GYRO_LSB_PER_DPS;
    /*
     * 当前 yaw 是陀螺仪 Z 轴积分值，短时间可用于角度环。
     * 长时间会有零偏漂移，需要时用 ICM42688_SetYaw() 手动归零。
     */
    g_imu.yaw_deg = PID_WrapDegrees(
        g_imu.yaw_deg + (g_imu.gyro_dps[2] * CAR_CONTROL_PERIOD_S));

    if (out != 0) {
        *out = g_imu;
    }

    return true;
}

const ICM42688_Data_t *ICM42688_GetData(void)
{
    return &g_imu;
}

void ICM42688_SetYaw(float yaw_deg)
{
    g_imu.yaw_deg = PID_WrapDegrees(yaw_deg);
}
