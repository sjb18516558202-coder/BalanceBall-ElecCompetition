/*********************************************************************************************************************
* MSPM0G3507 Opensource Library
* 文件名称          IMU
* 说明              IMU 陀螺仪模块驱动（UART 通信）
*                   修改内容：
*                   1. 添加零偏自动校准 IMU_BiasCalibrate()
*                   2. 积分时扣除零偏，减少漂移
*                   3. 配置命令改用动态 CRC 计算
*                   4. 添加 Yaw 重置函数
********************************************************************************************************************/

#include "IMU.h"

volatile float imu_angle = 0.0f;            // 角度 (度)
volatile float imu_dps   = 0.0f;            // 角速度 (度/秒)，已扣除零偏
volatile float imu_dps_raw_nobias = 0.0f;   // 原始角速度 (度/秒)，未扣除零偏
volatile float imu_yaw_sum = 0.0f;          // 角速度积分得到的累计偏航角，单位度
volatile float imu_bias_dps = 0.0f;         // 零偏校准值
volatile uint8_t imu_calibrated = 0;        // 校准完成标志
volatile uint32_t imu_rx_byte_count = 0;
volatile uint32_t imu_rx_frame_count = 0;
volatile uint32_t imu_rx_crc_error = 0;

static volatile int16_t gyro_angle_raw;     // 角度原始值 (0.1度/LSB)
static volatile int16_t gyro_dps_raw;       // 角速度原始值 (0.1 dps/LSB)
static volatile uint8_t  gyro_rx_done;      // 数据接收完成标志

#define IMU_REPORT_DT_S     ( 0.01f )       // 参考例程配置为 10ms 上报一次，按 0.01s 积分。
#define IMU_CALIB_SAMPLES   ( 200 )         // 零偏校准采样帧数（200帧 × 10ms = 2秒）

static void Gyro_ParseFrame(uint8_t data);
static void Gyro_ConfigReportRateRx(void);

/* ============================ 零偏自动校准 ============================ */
/*
 * 功能：小车静止状态下采样多帧角速度，计算零偏。
 *       校准期间会阻塞约 sample_count * 10ms。
 *       校准完成后自动清零 imu_yaw_sum。
 */

void IMU_BiasCalibrate(uint16_t sample_time_ms)
{
    uint16_t target_samples;
    float    sum_dps = 0.0f;
    uint16_t count = 0;
    uint8_t  frame_ok;
    uint32_t max_samples;

    (void)sample_time_ms;  // 直接用帧数控制，保留参数接口

    // 先清空 UART3 缓冲区中的旧数据，避免读到校准前的残留帧
    {
        uint8_t dummy;
        while(uart_query_byte(UART_3, &dummy) == ZF_TRUE);
    }
    gyro_rx_done = 0;

    max_samples = IMU_CALIB_SAMPLES;

    // 等待首帧到达，确保 IMU 数据链路正常
    {
        uint32_t timeout = 0;
        while(!gyro_rx_done && timeout < 500000)
        {
            IMU_UART_Handler();
            timeout++;
        }
        if(!gyro_rx_done)
        {
            // IMU 无数据，偏置保持为 0，标记未校准
            imu_calibrated = 0;
            imu_bias_dps = 0.0f;
            return;
        }
    }

    // 丢弃首帧（因为上面只确认了有数据，还没有真正处理帧内容）
    // 重置标志后开始正式采样
    gyro_rx_done = 0;
    sum_dps = 0.0f;
    count = 0;

    while(count < max_samples)
    {
        IMU_UART_Handler();
        frame_ok = IMU_Read();
        if(frame_ok)
        {
            // 累加未扣除零偏的原始角速度
            sum_dps += imu_dps_raw_nobias;
            count++;
        }
    }

    if(count > 0)
    {
        imu_bias_dps = sum_dps / (float)count;
    }
    else
    {
        imu_bias_dps = 0.0f;
    }

    imu_calibrated = 1;
    imu_yaw_sum = 0.0f;             // 校准后清零偏航角

    // 调试输出
    {
        char buf[64];
        sprintf(buf, "IMU Calib Done: bias=%.2f dps, samples=%u\r\n", imu_bias_dps, count);
        uart_write_string(UART_1, buf);
    }
}

/* ============================ Yaw 重置 ============================ */

void IMU_ResetYaw(void)
{
    __disable_irq();
    imu_yaw_sum = 0.0f;
    __enable_irq();
}

/* ============================ 初始化 ============================ */

void IMU_Init(void)
{
    // 先等待 IMU 模块上电稳定
    system_delay_ms(100);

    // 配置陀螺仪上报速率，UART3 初始化在 main.c 中完成。
    Gyro_ConfigReportRateRx();
    system_delay_ms(50);

    // 变量复位
    imu_angle = 0.0f;
    imu_dps   = 0.0f;
    imu_dps_raw_nobias = 0.0f;
    imu_yaw_sum = 0.0f;
    imu_bias_dps = 0.0f;
    imu_calibrated = 0;
    imu_rx_byte_count = 0;
    imu_rx_frame_count = 0;
    imu_rx_crc_error = 0;
}

/* ============================ 读取最新数据 ============================ */

uint8_t IMU_Read(void)
{
    if(gyro_rx_done)
    {
        gyro_rx_done = 0;

        // 转换为实际值 (0.1度/LSB, 0.1 dps/LSB)。
        imu_angle = gyro_angle_raw * 0.1f;
        imu_dps_raw_nobias = gyro_dps_raw * 0.1f;

        // 扣除零偏，得到修正后的角速度
        imu_dps = imu_dps_raw_nobias - imu_bias_dps;

        return 1;
    }
    return 0;
}

/* ============================ UART 数据处理 ============================ */

void IMU_UART_Handler(void)
{
    uint8_t data;

    while(uart_query_byte(UART_3, &data) == ZF_TRUE)
    {
        IMU_UART_ReceiveByte(data);
    }
}

void IMU_UART_ReceiveByte(uint8_t data)
{
    imu_rx_byte_count++;
    Gyro_ParseFrame(data);
}

/* ============================ 帧解析 ============================ */

static void Gyro_ParseFrame(uint8_t data)
{
    static uint8_t state = 0;
    static uint8_t frame[9];
    static uint8_t idx = 0;
    uint16_t crc_calc;
    uint16_t crc_recv;

    switch(state)
    {
        case 0:
            if(data == 0x0A)
            {
                idx = 0;
                frame[idx++] = data;
                state = 1;
            }
            break;

        case 1:
            if(data == 0x03)
            {
                frame[idx++] = data;
                state = 2;
            }
            else
            {
                state = 0;
            }
            break;

        case 2:
            if(data == 0x04)
            {
                frame[idx++] = data;
                state = 3;
            }
            else
            {
                state = 0;
            }
            break;

        case 3:
            frame[idx++] = data;

            if(idx >= 9)
            {
                crc_calc = CRC16(frame, 7);
                crc_recv = frame[7] | ((uint16_t)frame[8] << 8);

                if(crc_calc == crc_recv)
                {
                    gyro_angle_raw = (int16_t)(((uint16_t)frame[3] << 8) | frame[4]);
                    gyro_dps_raw   = (int16_t)(((uint16_t)frame[5] << 8) | frame[6]);

                    // 换算为实际值
                    imu_angle = gyro_angle_raw * 0.1f;
                    imu_dps_raw_nobias = gyro_dps_raw * 0.1f;

                    // 扣除零偏
                    imu_dps = imu_dps_raw_nobias - imu_bias_dps;

                    // 梯形积分（用扣除零偏后的角速度）
                    imu_yaw_sum += imu_dps * IMU_REPORT_DT_S;

                    gyro_rx_done = 1;
                    imu_rx_frame_count++;
                }
                else
                {
                    imu_rx_crc_error++;
                }

                state = 0;
            }
            break;

        default:
            state = 0;
            break;
    }
}

/* ============================ 配置 IMU 上报速率 ============================ */

static void Gyro_ConfigReportRateRx(void)
{
    uint8_t  idx = 0;
    uint8_t  frame[20];
    uint16_t crc;

    // 从站地址 0xAA，功能码 0x06（写单个寄存器）
    // 寄存器 0x0101 = 上报周期设置
    // 值 0x01 = 10ms 上报一次
    frame[idx++] = 0xAA;        // 从站地址
    frame[idx++] = 0x06;        // 功能码：写单个寄存器
    frame[idx++] = 0x01;        // 寄存器地址高字节
    frame[idx++] = 0x01;        // 寄存器地址低字节
    frame[idx++] = 0x01;        // 寄存器值（10ms 上报周期）

    // 动态计算 CRC16（Modbus RTU），替代原来的硬编码 CRC
    crc = CRC16(frame, idx);
    frame[idx++] = (uint8_t)(crc & 0xFF);       // CRC 低字节在前
    frame[idx++] = (uint8_t)((crc >> 8) & 0xFF); // CRC 高字节在后

    uart_write_buffer(UART_3, frame, idx);
}
