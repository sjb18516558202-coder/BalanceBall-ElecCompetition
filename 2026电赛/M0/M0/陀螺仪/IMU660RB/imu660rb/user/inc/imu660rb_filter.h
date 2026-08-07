/*********************************************************************************************************************
* 文件名称          imu660rb_filter
* 功能说明          IMU660RB 产品级滤波与姿态融合模块
* 适用场景          小车、移动机器人等带震动平台
*
* 设计思路：
* 1. 传感器层：硬件 DLPF + 软件中值滤波，先压尖峰和电机高频震动。
* 2. 补偿层：上电静止零偏校准 + 温度参考 + 静止在线零偏学习，尽量减小零漂/温漂。
* 3. 抗震层：加速度模长、jerk 和角速度联合判断震动/运动状态，动态调整低通和融合权重。
* 4. 融合层：内置 Mahony、Madgwick、Mahony+一维卡尔曼三种方案，默认使用小车更稳的 Mahony+Kalman。
*
* 注意：IMU660RB 是六轴 IMU，没有磁力计，也没有轮速/视觉约束，Yaw 只能短时间相对稳定，不能长期绝对不漂。
********************************************************************************************************************/

#ifndef _imu660rb_filter_h_
#define _imu660rb_filter_h_

#include "zf_common_typedef.h"

// 推荐采样周期：5ms = 200Hz。小车姿态足够快，同时方便软件滤波。
#define IMU660RB_FILTER_PERIOD_MS       (5U)
#define IMU660RB_FILTER_DT              (0.005f)

// 姿态融合算法选择：
// IMU660RB_FUSION_MAHONY         ：经典 Mahony 非线性互补滤波，运算量低，抗干扰能力好。
// IMU660RB_FUSION_MADGWICK       ：Madgwick 梯度下降 AHRS，参数少，嵌入式常见。
// IMU660RB_FUSION_MAHONY_KALMAN  ：Mahony 负责四元数/Yaw，一维 Kalman 优化 Roll/Pitch，默认推荐小车使用。
#define IMU660RB_FUSION_MAHONY          (0U)
#define IMU660RB_FUSION_MADGWICK        (1U)
#define IMU660RB_FUSION_MAHONY_KALMAN   (2U)

#ifndef IMU660RB_FUSION_ALGORITHM
#define IMU660RB_FUSION_ALGORITHM       IMU660RB_FUSION_MAHONY_KALMAN
#endif

typedef struct
{
    float ax_g;                         // 滤波后加速度 X，单位 g
    float ay_g;                         // 滤波后加速度 Y，单位 g
    float az_g;                         // 滤波后加速度 Z，单位 g

    float gx_dps;                       // 滤波、补偿后角速度 X，单位 °/s
    float gy_dps;                       // 滤波、补偿后角速度 Y，单位 °/s
    float gz_dps;                       // 滤波、补偿后角速度 Z，单位 °/s

    float roll_deg;                     // 最终横滚角，默认来自 Kalman 优化后的 Roll
    float pitch_deg;                    // 最终俯仰角，默认来自 Kalman 优化后的 Pitch
    float yaw_deg;                      // 航向角，六轴条件下为相对角度；本工程平装小车默认使用Z轴专用积分Yaw
    float yaw_quat_deg;                 // 四元数融合得到的Yaw，调试用；六轴条件下不作为小车默认航向
    float yaw_rate_dps;                 // 实际参与Yaw积分的Z轴角速度，已做死区/静止锁定，单位 °/s
    float yaw_bias_dps;                 // 运行中估计到的Z轴残余零偏，调试用，单位 °/s

    float roll_acc_deg;                 // 仅由加速度解算出的 Roll，调试用
    float pitch_acc_deg;                // 仅由加速度解算出的 Pitch，调试用
    float roll_mahony_deg;              // Mahony/Madgwick 四元数输出 Roll，调试用
    float pitch_mahony_deg;             // Mahony/Madgwick 四元数输出 Pitch，调试用
    float roll_kalman_deg;              // 一维 Kalman 输出 Roll，调试用
    float pitch_kalman_deg;             // 一维 Kalman 输出 Pitch，调试用

    float temperature_c;                // 温度，单位 ℃
    float acc_norm_g;                   // 加速度模长，单位 g
    float acc_weight;                   // 当前加速度融合可信度，0~1，越小表示越不相信加速度

    uint8 calibrated;                   // 是否完成上电校准
    uint8 static_flag;                  // 静止状态标志，可用于 ZARU 零角速度更新
    uint8 vibration_flag;               // 震动/冲击标志
    uint8 fusion_mode;                  // 当前使用的融合模式

    // 小车Yaw目标转角闭环控制调试量。用于“转90度不是90度”这种场景。
    uint8 turn_active;                  // 目标转角控制是否正在进行
    uint8 turn_done;                    // 目标转角是否完成
    float turn_target_deg;              // 本次目标相对转角，单位 °，例如 90 或 -90
    float turn_error_deg;               // 当前剩余误差，单位 °
    float turn_output;                  // 闭环控制输出，范围 -1~1，可映射到左右轮差速
}imu660rb_filter_data_t;


//===================================== 快速调用说明 =====================================
// 1. 初始化顺序：
//      imu660rb_init();                         // 先初始化底层 IMU660RB
//      imu660rb_filter_init();                  // 再初始化软件滤波模块
//      imu660rb_filter_calibrate(800);          // 小车静止约 4 秒，完成零偏校准
//      pit_ms_init(..., 5, callback, NULL);      // 定时器每 5ms 调用 imu660rb_filter_update(0.005f)
//
// 2. 小车运行时常用读取值：
//      imu660rb_filter_data.yaw_deg             // 平装小车转角，转 90 度主要看它
//      imu660rb_filter_data.yaw_rate_dps        // 参与积分后的转向角速度，可用于判断是否还在旋转
//      imu660rb_filter_data.gz_dps              // 滤波补偿后的Z轴角速度，适合做转向速度反馈
//      imu660rb_filter_data.roll_deg            // 车身左右倾斜角
//      imu660rb_filter_data.pitch_deg           // 车身前后俯仰角
//      imu660rb_filter_data.vibration_flag      // 震动/冲击标志
//      imu660rb_filter_data.static_flag         // 静止标志
//
// 3. 转90度控制建议：
//      imu660rb_filter_yaw_turn_start(90.0f);    // 启动目标转角
//      turn = imu660rb_filter_yaw_turn_update(); // 得到 -1~1 输出
//      左轮 = -turn * 最大PWM；右轮 = turn * 最大PWM；done 后停止电机。
//========================================================================================

extern imu660rb_filter_data_t imu660rb_filter_data;

void    imu660rb_filter_init                (void);
uint8   imu660rb_filter_calibrate           (uint16 sample_count);
void    imu660rb_filter_update              (float dt_s);
void    imu660rb_filter_reset_yaw           (void);

// 温漂系数接口：单位 °/s/℃。默认 0。若后续有高低温静止数据，可把拟合出来的三轴系数填进来。
void    imu660rb_filter_set_gyro_temp_coeff (float gx_k, float gy_k, float gz_k);

// Yaw比例校准接口：如果真实转90°，显示只有50°，可先设置 scale = 90/50 = 1.8。
// 建议先使用新版定时器更新和±1000dps量程测试，仍有固定比例误差时再用这个接口微调。
void    imu660rb_filter_set_yaw_scale       (float scale);

// 小车平装Yaw优化参数：
// deadband_dps：Z轴角速度小于该值时认为是零漂，直接不积分，常用 0.25~1.2。
// bias_alpha：静止/直行时在线学习Z轴残余零偏的速度，常用 0.0002~0.002。
// hold_enable：静止时是否锁住Yaw，1推荐开启。
void    imu660rb_filter_set_yaw_opt         (float deadband_dps, float bias_alpha, uint8 hold_enable);

// 清除运行中学习到的在线零偏，调试时可用。
void    imu660rb_filter_clear_online_bias   (void);

//===================================== 小车目标转角控制接口 =====================================
// 用法：转弯前调用 start(90)，主循环中不断读取 update() 返回值控制左右轮差速；done后停止电机。
// 这个接口会在转向过程中自动降低Yaw死区、禁止Z轴零偏学习，避免把慢速转角“吃掉”。
void    imu660rb_filter_yaw_turn_start      (float target_deg);
float   imu660rb_filter_yaw_turn_update     (void);
uint8   imu660rb_filter_yaw_turn_is_done    (void);
void    imu660rb_filter_yaw_turn_cancel     (void);

// 可选调参：kp越大转得越猛，kd越大刹车越早；min_output用于克服电机死区。
void    imu660rb_filter_yaw_turn_set_param  (float kp, float kd, float max_output, float min_output, float finish_error_deg, float finish_rate_dps);

#endif
