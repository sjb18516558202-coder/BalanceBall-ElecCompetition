/*********************************************************************************************************************
* MSPM0G3507 ICM20602 SPI + IPS200 Demo
* 功能：读取 ICM20602 六轴数据，进行产品级滤波/温漂抑制/震动抑制，并在 IPS200 屏幕和串口显示
********************************************************************************************************************/

#include "zf_common_headfile.h"
#include "icm20602_filter.h"

// 由 PIT 定时器固定 5ms 调用滤波，避免屏幕刷新/串口打印拖慢主循环导致 Yaw 积分偏小。
static volatile uint32 imu_update_count = 0;
static volatile uint8 imu_filter_enable = 0;

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     ICM20602 滤波定时器回调函数
// 参数说明     event               PIT定时器事件参数，本例未使用
// 参数说明     ptr                 用户指针，本例未使用
// 返回参数     void
// 使用说明     由 PIT 每 5ms 自动调用一次，内部调用 icm20602_filter_update()，保证IMU积分周期固定
// 注意事项     不要在该回调中刷新屏幕、printf 或执行复杂电机控制，避免中断/定时任务耗时过长
//-------------------------------------------------------------------------------------------------------------------
static void icm20602_filter_timer_callback(uint32 event, void *ptr)
{
    (void)event;
    (void)ptr;

    if(imu_filter_enable)
    {
        icm20602_filter_update(ICM20602_FILTER_DT);
        imu_update_count ++;
    }
}

// *************************** 例程硬件连接说明 ***************************
// 接入 ICM20602：
//      SCL/SPC/SCLK         查看 zf_device_icm20602.h 中 ICM20602_SPC_PIN 宏定义，默认 B23
//      SDA/SDI/MOSI         查看 zf_device_icm20602.h 中 ICM20602_SDI_PIN 宏定义，默认 B22
//      SA0/SDO/MISO         查看 zf_device_icm20602.h 中 ICM20602_SDO_PIN 宏定义，默认 B21
//      CS/NCS               查看 zf_device_icm20602.h 中 ICM20602_CS_PIN  宏定义，默认 B19
//      GND                  电源地 GND
//      VCC                  3.3V 电源，不能接 5V
//
// 接入 IPS200：
//      SCL                  查看 zf_device_ips200.h 中 IPS200_SCL_PIN_SPI 宏定义，默认 A12
//      SDA                  查看 zf_device_ips200.h 中 IPS200_SDA_PIN_SPI 宏定义，默认 A9
//      RST                  查看 zf_device_ips200.h 中 IPS200_RST_PIN_SPI 宏定义，默认 A7
//      DC                   查看 zf_device_ips200.h 中 IPS200_DC_PIN_SPI  宏定义，默认 A15
//      CS                   查看 zf_device_ips200.h 中 IPS200_CS_PIN_SPI  宏定义，默认 A8
//      BLK                  查看 zf_device_ips200.h 中 IPS200_BLk_PIN_SPI 宏定义，默认 A13
//      GND                  电源地 GND
//      VCC                  3.3V 电源，不能接 5V


static void icm20602_ips200_show_static_ui(void)
{
    ips200_clear();
    ips200_set_font(IPS200_8X16_FONT);
    ips200_set_color(RGB565_BLACK, RGB565_WHITE);

    ips200_show_string(0,   0, "ICM20602 PRODUCT AHRS");
    ips200_show_string(0,  24, "Roll :");
    ips200_show_string(0,  48, "Pitch:");
    ips200_show_string(0,  72, "Yaw  :");

    ips200_show_string(0, 104, "Gx dps:");
    ips200_show_string(0, 128, "Gy dps:");
    ips200_show_string(0, 152, "Gz dps:");

    ips200_show_string(0, 184, "Err  :");
    ips200_show_string(0, 208, "Out  :");
    ips200_show_string(168, 184, "State:");
    ips200_show_string(168, 208, "ALG:");
}

static void icm20602_ips200_show_data(void)
{
    ips200_set_color(RGB565_BLACK, RGB565_WHITE);

    // Roll / Pitch / Yaw 单位为 °。Yaw 无磁力计约束，长时间会有少量漂移。
    ips200_show_float(64,  24, icm20602_filter_data.roll_deg,  4, 1);
    ips200_show_float(64,  48, icm20602_filter_data.pitch_deg, 4, 1);
    ips200_show_float(64,  72, icm20602_filter_data.yaw_deg,   4, 1);

    // 滤波、零偏、温漂补偿后的角速度，单位 °/s。
    ips200_show_float(72, 104, icm20602_filter_data.gx_dps, 4, 1);
    ips200_show_float(72, 128, icm20602_filter_data.gy_dps, 4, 1);
    ips200_show_float(72, 152, icm20602_filter_data.gz_dps, 4, 1);

    // 转90度闭环调试：Err 为剩余角度误差，Out 为建议转向输出 -1~1。
    ips200_show_float(64, 184, icm20602_filter_data.turn_error_deg, 4, 1);
    ips200_show_float(64, 208, icm20602_filter_data.turn_output, 1, 3);

    if(ICM20602_FUSION_MAHONY_KALMAN == icm20602_filter_data.fusion_mode)
    {
        ips200_show_string(208, 208, "M+KF");
    }
    else if(ICM20602_FUSION_MADGWICK == icm20602_filter_data.fusion_mode)
    {
        ips200_show_string(208, 208, "MADG");
    }
    else
    {
        ips200_show_string(208, 208, "MAHO");
    }

    if(icm20602_filter_data.vibration_flag)
    {
        ips200_set_color(RGB565_RED, RGB565_WHITE);
        ips200_show_string(224, 184, "VIB ");
    }
    else if(icm20602_filter_data.static_flag)
    {
        ips200_set_color(RGB565_GREEN, RGB565_WHITE);
        ips200_show_string(224, 184, "STOP");
    }
    else
    {
        ips200_set_color(RGB565_BLUE, RGB565_WHITE);
        ips200_show_string(224, 184, "MOVE");
    }
    ips200_set_color(RGB565_BLACK, RGB565_WHITE);
}


int main (void)
{
    uint32 last_display_count = 0;
    uint32 last_print_count = 0;
    uint32 now_count = 0;

    clock_init(SYSTEM_CLOCK_80M);                                               // 时钟配置及系统初始化<务必保留>
    debug_init();                                                              // 调试串口信息初始化

    ips200_init(IPS200_TYPE_SPI);                                               // 初始化 IPS200 屏幕
    ips200_set_dir(IPS200_CROSSWISE);                                           // 横屏显示，分辨率方向更适合显示数据
    ips200_set_color(RGB565_BLACK, RGB565_WHITE);
    ips200_clear();
    ips200_show_string(0, 0, "ICM20602 init...");

    while(1)
    {
        if(icm20602_init())
        {
            printf("\r\nicm20602 init error. check VCC/GND/SCLK/MOSI/MISO/CS.");
            ips200_set_color(RGB565_RED, RGB565_WHITE);
            ips200_show_string(0, 0, "ICM20602 init error!    ");
            ips200_show_string(0, 24, "Check wiring and power.");
            ips200_show_string(0, 48, "VCC GND SCLK MOSI MISO CS");
            system_delay_ms(500);
        }
        else
        {
            printf("\r\nicm20602 init success.");
            ips200_set_color(RGB565_BLACK, RGB565_WHITE);
            ips200_show_string(0, 0, "ICM20602 init success.  ");
            system_delay_ms(500);
            break;
        }
    }

    icm20602_filter_init();

    // 产品级使用时，上电校准非常重要：让小车静止约 4 秒，得到陀螺仪零偏和温度基准。
    ips200_clear();
    ips200_set_color(RGB565_BLACK, RGB565_WHITE);
    ips200_show_string(0, 0,  "Calibrating gyro...");
    ips200_show_string(0, 24, "Keep the car STILL");
    ips200_show_string(0, 48, "about 4 seconds");
    printf("\r\nICM20602 filter calibrating, keep car still...");
    icm20602_filter_calibrate(800);
    printf("\r\nICM20602 filter calibrate done.");

    icm20602_ips200_show_static_ui();

    // 关键修改：滤波更新放到 5ms PIT 定时器中，真正做到固定 200Hz。
    // 这样屏幕刷新、printf 或小车其他控制逻辑变慢，也不会让 Yaw 积分角度偏小。
    imu_filter_enable = 1;
    pit_ms_init(PIT_TIM_G0, ICM20602_FILTER_PERIOD_MS, icm20602_filter_timer_callback, NULL);

    // 如果要让小车自己转90度，不要开环延时转弯，也不要等Yaw到90才突然刹车。
    // 正确做法：调用 icm20602_filter_yaw_turn_start(90.0f)，然后用 turn_update() 返回的 -1~1 控制左右轮差速。
    // 示例见工程根目录《Yaw转90度闭环控制说明.txt》。这里默认不自动启动，避免上电后小车自己动。


    /*
    ----------------------------------------------------------------------------------------------------------------
    电机配套调用例程 1：读取 Yaw 做短距离转角判断
    ----------------------------------------------------------------------------------------------------------------
    // 当前车头方向作为 0 度
    icm20602_filter_reset_yaw();

    // 后续在控制循环中读取 yaw_deg，就能看到相对当前方向转了多少度
    float yaw = icm20602_filter_data.yaw_deg;

    ----------------------------------------------------------------------------------------------------------------
    电机配套调用例程 2：让小车闭环转 90 度
    ----------------------------------------------------------------------------------------------------------------
    #define TURN_PWM_MAX   (3000)     // 这里换成你自己电机允许的最大PWM

    icm20602_filter_yaw_turn_start(90.0f);       // 正90度，反方向用 -90.0f

    while(!icm20602_filter_yaw_turn_is_done())
    {
        float turn = icm20602_filter_yaw_turn_update();

        // 下面两个函数是示例名，需要替换成你自己工程里的电机控制函数。
        // 如果发现方向反了，就把左右轮符号对调。
        motor_left_set_speed ((int16)(-turn * TURN_PWM_MAX));
        motor_right_set_speed((int16)( turn * TURN_PWM_MAX));
    }

    motor_left_set_speed(0);
    motor_right_set_speed(0);

    ----------------------------------------------------------------------------------------------------------------
    电机配套调用例程 3：常用数据读取
    ----------------------------------------------------------------------------------------------------------------
    float yaw_angle = icm20602_filter_data.yaw_deg;          // 当前累计转角，单位：度
    float yaw_speed = icm20602_filter_data.yaw_rate_dps;     // 实际用于Yaw积分的角速度，单位：度/秒
    float gz_speed  = icm20602_filter_data.gz_dps;           // 滤波后的Z轴角速度，单位：度/秒
    float roll      = icm20602_filter_data.roll_deg;         // 车身左右倾斜角
    float pitch     = icm20602_filter_data.pitch_deg;        // 车身前后俯仰角
    uint8 is_vib    = icm20602_filter_data.vibration_flag;   // 1表示当前震动/冲击比较明显
    */

    while(true)
    {
        now_count = imu_update_count;

        if((now_count - last_display_count) >= 10U)                             // 约 50ms 刷新一次屏幕
        {
            last_display_count = now_count;
            icm20602_ips200_show_data();
        }

        if((now_count - last_print_count) >= 20U)                                // 约 100ms 打印一次串口
        {
            last_print_count = now_count;
            printf("\r\nroll=%d.%d pitch=%d.%d yaw=%d.%d yrate=%d.%d ybias=%d.%d gz=%d.%d err=%d.%d out=%d.%d state=%s",
                   (int)icm20602_filter_data.roll_deg,  (int)(icm20602_filter_data.roll_deg  * 10.0f) % 10,
                   (int)icm20602_filter_data.pitch_deg, (int)(icm20602_filter_data.pitch_deg * 10.0f) % 10,
                   (int)icm20602_filter_data.yaw_deg,   (int)(icm20602_filter_data.yaw_deg   * 10.0f) % 10,
                   (int)icm20602_filter_data.yaw_rate_dps, (int)(icm20602_filter_data.yaw_rate_dps * 10.0f) % 10,
                   (int)icm20602_filter_data.yaw_bias_dps, (int)(icm20602_filter_data.yaw_bias_dps * 10.0f) % 10,
                   (int)icm20602_filter_data.gz_dps,    (int)(icm20602_filter_data.gz_dps    * 10.0f) % 10,
                   (int)icm20602_filter_data.turn_error_deg, (int)(icm20602_filter_data.turn_error_deg * 10.0f) % 10,
                   (int)icm20602_filter_data.turn_output, (int)(icm20602_filter_data.turn_output * 10.0f) % 10,
                   icm20602_filter_data.vibration_flag ? "VIB" : (icm20602_filter_data.static_flag ? "STOP" : "MOVE"));
        }

        // 主循环只做显示/控制，滤波更新由定时器保证周期。
        system_delay_ms(1);
    }
}
