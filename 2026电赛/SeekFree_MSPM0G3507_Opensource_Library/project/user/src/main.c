/*********************************************************************************************************************
* MSPM0G3507 Opensource Library 即（MSPM0G3507 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 本文件是 MSPM0G3507 开源库的一部分
*
* MSPM0G3507 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 本开源库的发布是希望它能发挥作用，但并未对其作任何的保证
* 甚至没有隐含的适销性或适合特定用途的保证
* 更多细节请参见 GPL
*
* 您应该在收到本开源库的同时收到一份 GPL 的副本
* 如果没有，请参阅<https://www.gnu.org/licenses/>
*
* 额外注明：
* 本开源库使用 GPL3.0 开源许可证协议 以上许可申明为译文版本
* 许可申明英文版在 libraries/doc 文件夹下的 GPL3_permission_statement.txt 文件中
* 许可证副本在 libraries 文件夹下 即该文件夹下的 LICENSE 文件
* 欢迎各位使用并传播本程序 但修改内容时必须保留逐飞科技的版权声明（即本声明）
*
* 文件名称          main
* 公司名称          成都逐飞科技有限公司
* 版本信息          查看 libraries/doc 文件夹内 version 文件 版本说明
* 开发环境          MDK 5.37
* 适用平台          MSPM0G3507
* 店铺链接          https://seekfree.taobao.com/
*
* 修改说明：
*   1. 启动时自动校准 IMU 零偏（小车需静止）
*   2. 用 1ms PIT 的 run_time_ms 代替写死的 +=10 计时
*   3. 停车前主动读取 IMU，防止积分停滞
*   4. 增加 DPS 死区过滤微小漂移
********************************************************************************************************************/

#include "zf_common_headfile.h"
#include "gray_track.h"
#include "motorsetspeed.h"
#include "motor.h"
#include "IMU.h"
// 打开新的工程或者工程移动了位置务必执行以下操作
// 第一步 关闭上面所有打开的文件
// 第二步 project->clean  等待下方进度条走完

// **************************** 代码区域 ****************************

// 启动开关：默认使用 A30，上拉输入，按下时为低电平。若实际引脚/电平不同，只改这里。
#define CAR_START_SWITCH_PIN          ( A30 )
#define CAR_START_SWITCH_PRESSED      ( GPIO_LOW )

// 陀螺仪一圈停车参数
#define GYRO_STOP_ENABLE              ( 1 )
#define GYRO_STOP_MIN_TIME_MS         ( 500 )         // 发车后最短运行时间，防止起步误触发停车
#define GYRO_STOP_ANGLE_DEG           ( 330.0f )      // 累计偏航角阈值（度），接近一圈时停车
#define GYRO_DPS_DEADZONE             ( 0.3f )        // 角速度死区：|dps| 小于此值视为静止，不积分

static volatile uint32 run_time_ms = 0;        // 按键发车后的行驶总时间，停车后保持不变。
static volatile uint8  run_time_enable = 0;    // 1ms PIT 计时开关。

static void run_time_pit_handler(uint32 event, void *ptr)
{
    (void)event;
    (void)ptr;

    if(run_time_enable)
    {
        run_time_ms++;
    }
}

int main (void)
{
    char  uart_buf[64];
    uint8 car_started = 0;
    uint8 car_stopped = 0;
    uint32 gyro_run_start_ms = 0;              // 发车时刻的 run_time_ms
    uint32 gyro_debug_last_ms = 0;
    float  gyro_start_yaw = 0.0f;
    float  gyro_run_yaw = 0.0f;
    uint8  imu_updated;

    clock_init(SYSTEM_CLOCK_80M);       // 时钟配置及系统初始化<务必保留>
    debug_init();                       // 调试串口信息初始化
    system_delay_ms(1000);              // 上电延时，等待驱动板和外设稳定

    // UART1 用作调试输出，可接无线串口/USB-TTL/串口助手，不参与电机控制。
    uart_init(UART_1, 115200, UART1_TX_B6, UART1_RX_B7);
    system_delay_ms(50);

    // UART2 用作电机驱动板通信：B15(TX)->驱动板RX，B16(RX)->驱动板TX，GND共地。
    uart_init(UART_2, 115200, UART2_TX_B15, UART2_RX_B16);
    system_delay_ms(50);

    // UART3 用作陀螺仪通信：B12(TX)->陀螺仪RX，B13(RX)->陀螺仪TX。
    uart_init(UART_3, 115200, UART3_TX_B12, UART3_RX_B13);
    uart_set_interrupt_config(UART_3, UART_INTERRUPT_CONFIG_RX_ENABLE);
    system_delay_ms(50);

    // 初始化陀螺仪上报
    IMU_Init();
    system_delay_ms(50);

    // ======== 陀螺仪零偏自动校准（小车必须保持静止！）========
    uart_write_string(UART_1, "IMU Bias Calibrating... Keep car STILL!\r\n");
    IMU_BiasCalibrate(2000);   // 采样约 2 秒（200 帧 × 10ms）
    if(imu_calibrated)
    {
        sprintf(uart_buf, "IMU Calib OK: bias=%.2f dps\r\n", imu_bias_dps);
        uart_write_string(UART_1, uart_buf);
    }
    else
    {
        uart_write_string(UART_1, "IMU Calib FAILED! Check UART3 wiring.\r\n");
    }

    // 电机驱动初始化：闭环模式、停车、PID 参数。驱动板需要先刷闭环固件。
    Motor_init();
    system_delay_ms(50);

    // 初始化 8 路数字灰度循迹模块。实际 OUT 引脚在 gray_track.h 中填写。
    gray_track_init();

    // 1ms PIT 计时：按键发车时开启，停车时关闭，用于显示行驶总时间。
    pit_ms_init(PIT_TIM_G12, 1, run_time_pit_handler, NULL);

    // 启动开关用于防止上电后小车立即运动，按下后才开始循迹。
    gpio_init(CAR_START_SWITCH_PIN, GPI, GPIO_HIGH, GPI_PULL_UP);

    // IPS200 屏幕用于现场调试灰度和陀螺仪停车状态。
    ips200_init(IPS200_TYPE_SPI);
    ips200_set_color(RGB565_WHITE, RGB565_BLACK);
    ips200_clear();

    ips200_show_string(0, 16 * 1, "Gray:");
    ips200_show_string(0, 16 * 2, "Raw:");
    ips200_show_string(0, 16 * 3, "Err:");
    ips200_show_string(0, 16 * 4, "Lost:");
    ips200_show_string(0, 16 * 5, "Angle:");
    ips200_show_string(0, 16 * 6, "Yaw:");
    ips200_show_string(0, 16 * 7, "Cnt:");
    ips200_show_string(0, 16 * 8, "Stop:");
    ips200_show_string(0, 16 * 9, "Byte:");
    ips200_show_string(0, 16 * 10, "CrcE:");
    ips200_show_string(0, 16 * 11, "Bias:");
    ips200_show_string(0, 16 * 12, "Time:");

    while(true)
    {
        // ---- 处理 IMU 串口数据（中断收数为主，这里补收遗漏字节）----
        IMU_UART_Handler();

        if(!car_started && gpio_get_level(CAR_START_SWITCH_PIN) == CAR_START_SWITCH_PRESSED)
        {
            car_started = 1;
            car_stopped = 0;
            gyro_run_start_ms = run_time_ms;
            gyro_debug_last_ms = run_time_ms;

            // 重新清零偏航角，从发车瞬间开始积分
            gyro_start_yaw = imu_yaw_sum;
            gyro_run_yaw = 0.0f;

            run_time_ms = 0;
            run_time_enable = 1;
            system_delay_ms(20);        // 简单消抖，避免按键抖动造成误判。
            ips200_clear();             // 启动后关闭屏幕显示，减少跑车时的刷新耗时。
            gpio_init(IPS200_BLk_PIN_SPI, GPO, GPIO_LOW, GPO_PUSH_PULL);
        }

        if(car_started)
        {
            if(car_stopped)
            {
                Motor_Stop_All();
            }
            else
            {
                // 灰度循迹主控制：读取 8 路数字灰度，计算偏差，并通过 UART2 给驱动板发送速度。
                gray_track_run();

#if GYRO_STOP_ENABLE
                // 读取 IMU 最新数据（数据可能已经在 ISR 中更新过，这里确认标志位）
                imu_updated = IMU_Read();

                // 用 1ms PIT 提供的时间计算偏航角，替代原来的 +=10
                // 偏航角已在 ISR 中实时积分，这里只需计算差值
                gyro_run_yaw = imu_yaw_sum - gyro_start_yaw;

                // 每 500ms 输出一次调试信息
                if((run_time_ms - gyro_debug_last_ms) >= 500)
                {
                    gyro_debug_last_ms = run_time_ms;
                    sprintf(uart_buf, "GYRO frm=%u dps=%.1f yaw=%.1f bias=%.2f stop=%d t=%ums\r\n",
                        imu_rx_frame_count, imu_dps, gyro_run_yaw, imu_bias_dps, car_stopped, run_time_ms);
                    uart_write_string(UART_1, uart_buf);
                }

                // 停车判断：累计偏航角超过阈值
                if(run_time_ms > GYRO_STOP_MIN_TIME_MS &&
                    (gyro_run_yaw >= GYRO_STOP_ANGLE_DEG || gyro_run_yaw <= -GYRO_STOP_ANGLE_DEG))
                {
                    car_stopped = 1;
                    run_time_enable = 0;
                    Motor_Stop_All();
                    gpio_init(IPS200_BLk_PIN_SPI, GPO, GPIO_HIGH, GPO_PUSH_PULL);
                    ips200_clear();
                    ips200_show_string(0, 16 * 1, "Final Angle:");
                    ips200_show_int(16 * 8, 16 * 1, (int)gyro_run_yaw, 6);
                    ips200_show_string(0, 16 * 2, "Final Time:");
                    ips200_show_int(16 * 8, 16 * 2, run_time_ms, 6);
                    ips200_show_string(16 * 14, 16 * 2, "ms");

                    sprintf(uart_buf, "STOP! yaw=%.1f deg, time=%lu ms\r\n", gyro_run_yaw, run_time_ms);
                    uart_write_string(UART_1, uart_buf);
                }
#endif
            }
        }
        else
        {
            // 未按启动开关前保持停车，让陀螺仪静止稳定，同时仍读取灰度方便调试。
            Motor_Stop_All();
            gray_track_read();

            // 保持校准状态可见
            gyro_run_yaw = imu_yaw_sum - gyro_start_yaw;
        }

        if(!car_started)
        {
            ips200_show_int(16 * 6, 16 * 1, gray_track_value[0] * 10000000 + gray_track_value[1] * 1000000 +
                gray_track_value[2] * 100000 + gray_track_value[3] * 10000 + gray_track_value[4] * 1000 +
                gray_track_value[5] * 100 + gray_track_value[6] * 10 + gray_track_value[7], 8);
            ips200_show_int(16 * 6, 16 * 2, gray_track_raw[0] * 10000000 + gray_track_raw[1] * 1000000 +
                gray_track_raw[2] * 100000 + gray_track_raw[3] * 10000 + gray_track_raw[4] * 1000 +
                gray_track_raw[5] * 100 + gray_track_raw[6] * 10 + gray_track_raw[7], 8);
            ips200_show_int(16 * 6, 16 * 3, gray_track_error, 6);
            ips200_show_int(16 * 6, 16 * 4, gray_track_lost_line, 1);
            ips200_show_int(16 * 6, 16 * 5, (int)imu_angle, 6);
            ips200_show_int(16 * 6, 16 * 6, (int)gyro_run_yaw, 6);
            ips200_show_int(16 * 6, 16 * 7, imu_rx_frame_count, 6);
            ips200_show_int(16 * 6, 16 * 8, car_stopped, 1);
            ips200_show_int(16 * 6, 16 * 9, imu_rx_byte_count, 6);
            ips200_show_int(16 * 6, 16 * 10, imu_rx_crc_error, 6);
            ips200_show_int(16 * 6, 16 * 11, (int)imu_bias_dps, 6);
            ips200_show_int(16 * 6, 16 * 12, run_time_ms, 6);
        }

        system_delay_ms(10);
    }
}
