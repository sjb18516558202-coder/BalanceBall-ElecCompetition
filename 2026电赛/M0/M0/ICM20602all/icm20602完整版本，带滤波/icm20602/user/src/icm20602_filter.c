/*********************************************************************************************************************
* 文件名称          icm20602_filter
* 功能说明          ICM20602 产品级滤波与姿态融合模块
*
* 本文件把“小车上 IMU 会遇到的问题”按工程顺序处理：
* 原始数据 -> 中值去尖峰 -> 零偏/温漂补偿 -> 可选陷波 -> 自适应低通 -> 静止零角速度更新 -> 姿态融合。
由于本文是作者上班听同事研究瞎写的，有不懂的丢给ai，ai能给你解答，后续有缘解答。上班真的很没有个人时间。。。。。。。。。。。。。。。。。。。
********************************************************************************************************************/

#include "zf_common_headfile.h"
#include "icm20602_filter.h"
#include <math.h>

#ifndef M_PI
#define M_PI                                (3.14159265358979323846f)//转弧度的参数
#endif

//======================================= 可根据小车机械结构微调的参数 ==============================================
#define ICM20602_CALIBRATE_DELAY_MS         (5U)                                // 校准采样间隔，和主循环采样周期一致
#define ICM20602_STATIC_GYRO_LIMIT_DPS      (2.0f)                              // 三轴角速度和小于该值，并且加速度接近 1g，认为小车静止
#define ICM20602_STATIC_ACC_LIMIT_G         (0.06f)                             // 静止时 |acc_norm - 1g| 允许范围
#define ICM20602_VIB_ACC_LIMIT_G            (0.14f)                             // 震动/冲击判断阈值，越小越敏感
#define ICM20602_VIB_JERK_LIMIT_G           (0.12f)                             // 加速度模长变化过快，认为存在震动
#define ICM20602_VIB_GYRO_LIMIT_DPS         (85.0f)                             // 角速度瞬间过大时，也认为存在冲击/强震动

#define ICM20602_NORMAL_ACC_CUTOFF_HZ       (22.0f)                             // 正常状态下加速度软件低通截止频率
#define ICM20602_VIB_ACC_CUTOFF_HZ          (7.0f)                              // 震动状态下降低加速度截止频率
#define ICM20602_NORMAL_GYRO_CUTOFF_HZ      (55.0f)                             // 正常状态下陀螺仪软件低通截止频率，转向角度测试不能过低
#define ICM20602_VIB_GYRO_CUTOFF_HZ         (30.0f)                             // 震动状态下降低陀螺仪截止频率，保留转向响应

#define ICM20602_MAHONY_KP                  (2.2f)                              // Mahony 比例增益
#define ICM20602_MAHONY_KI                  (0.05f)                             // Mahony 积分增益，过大容易震动
#define ICM20602_MADGWICK_BETA              (0.055f)                            // Madgwick 梯度下降增益，越大越快但越抖
#define ICM20602_ONLINE_BIAS_ALPHA          (0.0012f)                           // 静止在线零偏学习速度，越小越稳
#define ICM20602_STATIC_GYRO_DAMP_ALPHA     (0.08f)                             // 静止时把残余角速度缓慢拉回 0，降低 Yaw 漂移

// 平装小车专用 Yaw 优化：
// ICM20602 六轴没有磁力计，Yaw 没有绝对参考，只能靠 Z 轴陀螺积分。
// 因此小车产品上必须加入死区、静止锁定和直行零偏学习，避免停车或直行时慢慢飘。
#define ICM20602_YAW_DEADBAND_DPS           (0.65f)                             // Z轴角速度死区，小车一般 0.4~1.0dps
#define ICM20602_YAW_BIAS_ALPHA             (0.00045f)                          // 直行/近似静止时Z轴零偏学习速度
#define ICM20602_YAW_BIAS_LIMIT_DPS         (8.0f)                              // 在线Z轴零偏保护限幅，防止误学习
#define ICM20602_YAW_NO_TURN_GYRO_XY_DPS    (5.0f)                              // X/Y轴角速度小于该值，才允许判断为直行/无转向
#define ICM20602_YAW_NO_TURN_ACC_ERR_G      (0.18f)                             // 加速度模长偏离1g不太大，才允许直行零偏学习
#define ICM20602_YAW_NO_TURN_CONFIRM_COUNT  (10U)                               // 连续确认约50ms后才学习，避免刚转向结束误判

// 目标转角/转90度场景专用参数。转向过程中不能用太大的死区，否则减速尾段会被吃掉。
#define ICM20602_YAW_TURN_DYNAMIC_DPS       (1.2f)                              // 大于该角速度认为正在真实转向，自动进入小死区积分
#define ICM20602_YAW_TURN_DEADBAND_DPS      (0.06f)                             // 转向时的积分死区，不能太大
#define ICM20602_YAW_TURN_FINISH_COUNT      (6U)                                // 连续约30ms满足误差/速度条件才认为到位
#define ICM20602_YAW_TURN_DEFAULT_KP        (0.030f)                            // 转角闭环P系数，输出范围 -1~1
#define ICM20602_YAW_TURN_DEFAULT_KD        (0.010f)                            // 转角闭环D系数，按角速度提前刹车
#define ICM20602_YAW_TURN_DEFAULT_MAX_OUT   (0.65f)                             // 最大转向输出，避免太猛导致过冲
#define ICM20602_YAW_TURN_DEFAULT_MIN_OUT   (0.18f)                             // 最小转向输出，克服电机死区
#define ICM20602_YAW_TURN_FINISH_ERR_DEG    (1.5f)                              // 到位角度误差阈值
#define ICM20602_YAW_TURN_FINISH_RATE_DPS   (8.0f)                              // 到位时角速度也要足够小，否则认为还会过冲

// 二阶陷波器默认关闭。只有确认电机/底盘存在固定频率窄带震动时再打开；否则低通 + 权重门控更稳。
#define ICM20602_NOTCH_ENABLE               (0U)
#define ICM20602_NOTCH_FREQ_HZ              (65.0f)                             // 需要实测电机震动主频后再改
#define ICM20602_NOTCH_Q                    (4.0f)
//====================================================================================================================

typedef struct
{
    float x;
    float y;
    float z;
}icm20602_vec3f_t;

typedef struct
{
    float angle;                                                                // 角度状态，单位 °
    float bias;                                                                 // 陀螺零偏状态，单位 °/s
    float rate;                                                                 // 去偏后的角速度，单位 °/s
    float p00;
    float p01;
    float p10;
    float p11;
    float q_angle;
    float q_bias;
    float r_measure;
}icm20602_kalman1d_t;

typedef struct
{
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
    float x1;
    float x2;
    float y1;
    float y2;
}icm20602_biquad_t;

icm20602_filter_data_t icm20602_filter_data;

static icm20602_vec3f_t gyro_base_bias_dps;                                     // 上电静止校准得到的陀螺零偏
static icm20602_vec3f_t gyro_online_bias_dps;                                   // 运行中静止状态下慢速学习的零偏
static icm20602_vec3f_t gyro_temp_k_dps_per_c = {0.0f, 0.0f, 0.0f};             // 温漂系数，后续可实测填写
static float temperature_base_c = 25.0f;                                        // 校准时温度
static float yaw_scale = 1.0227f;                                                    // Yaw比例校准系数，用于修正固定比例误差
//  校准方法                                 yaw_scale = 实际角度 / 显示角度;
static float car_yaw_deg = 0.0f;                                                  // 平装小车专用Yaw，直接由Z轴角速度积分
static float yaw_deadband_dps = ICM20602_YAW_DEADBAND_DPS;                       // 可运行时调整的Yaw死区
static float yaw_bias_alpha = ICM20602_YAW_BIAS_ALPHA;                           // 可运行时调整的Z轴在线零偏学习速度
static uint8 yaw_hold_enable = 1;                                                // 静止时锁住Yaw，产品级小车建议开启
static uint16 yaw_no_turn_count = 0;                                             // 直行/无转向连续确认计数

static uint8 yaw_turn_active = 0;                                                // 目标转角控制状态
static uint8 yaw_turn_done = 0;
static uint16 yaw_turn_finish_count = 0;
static float yaw_turn_start_deg = 0.0f;
static float yaw_turn_target_deg = 0.0f;
static float yaw_turn_error_deg = 0.0f;
static float yaw_turn_output = 0.0f;
static float yaw_turn_kp = ICM20602_YAW_TURN_DEFAULT_KP;
static float yaw_turn_kd = ICM20602_YAW_TURN_DEFAULT_KD;
static float yaw_turn_max_output = ICM20602_YAW_TURN_DEFAULT_MAX_OUT;
static float yaw_turn_min_output = ICM20602_YAW_TURN_DEFAULT_MIN_OUT;
static float yaw_turn_finish_error_deg = ICM20602_YAW_TURN_FINISH_ERR_DEG;
static float yaw_turn_finish_rate_dps = ICM20602_YAW_TURN_FINISH_RATE_DPS;

static icm20602_vec3f_t acc_lpf_g;
static icm20602_vec3f_t gyro_lpf_dps;
static float last_acc_norm_g = 1.0f;
static uint16 static_count = 0;

static float q0 = 1.0f;                                                         // 姿态四元数
static float q1 = 0.0f;
static float q2 = 0.0f;
static float q3 = 0.0f;
static float integral_x = 0.0f;
static float integral_y = 0.0f;
static float integral_z = 0.0f;

static icm20602_kalman1d_t roll_kalman;
static icm20602_kalman1d_t pitch_kalman;

static int16 acc_hist[3][3];                                                    // 三轴加速度最近 3 次原始值，用于中值去尖峰
static int16 gyro_hist[3][3];                                                   // 三轴陀螺仪最近 3 次原始值，用于中值去尖峰
static uint8 hist_index = 0;
static uint8 hist_filled = 0;

#if ICM20602_NOTCH_ENABLE
static icm20602_biquad_t acc_notch[3];
static icm20602_biquad_t gyro_notch[3];
#endif

static float icm20602_abs_f(float x)
{
    return (0.0f <= x) ? x : -x;
}

static float icm20602_clamp_f(float x, float min_value, float max_value)
{
    if(x < min_value)
    {
        return min_value;
    }
    if(x > max_value)
    {
        return max_value;
    }
    return x;
}

static float icm20602_sign_f(float x)
{
    if(x > 0.0f)
    {
        return 1.0f;
    }
    else if(x < 0.0f)
    {
        return -1.0f;
    }
    return 0.0f;
}

static float icm20602_turn_error_f(float target, float current)
{
    // 本工程的 car_yaw_deg 是连续相对角度，不限制在±180。这里直接求差，适合转90/180/270。
    return target - current;
}

static float icm20602_apply_deadband_f(float x, float deadband)
{
    if(deadband < 0.0f)
    {
        deadband = -deadband;
    }

    if(icm20602_abs_f(x) <= deadband)
    {
        return 0.0f;
    }
    return x;
}

static int16 icm20602_median3_int16(int16 a, int16 b, int16 c)
{
    if((a <= b && b <= c) || (c <= b && b <= a))
    {
        return b;
    }
    if((b <= a && a <= c) || (c <= a && a <= b))
    {
        return a;
    }
    return c;
}

static float icm20602_lpf_alpha(float cutoff_hz, float dt_s)
{
    float rc;
    float alpha;

    if(cutoff_hz < 0.1f)
    {
        cutoff_hz = 0.1f;
    }
    rc = 1.0f / (2.0f * M_PI * cutoff_hz);
    alpha = dt_s / (rc + dt_s);
    return icm20602_clamp_f(alpha, 0.0f, 1.0f);
}

static float icm20602_temp_to_c(int16 temp_raw)
{
    // ICM20602/MPU 系列常用温度换算公式：Temp = raw / 326.8 + 25，主要用于温漂补偿参考。参考文献请直接看知网，有点忘记哪篇文章了，
		//v1.2,想起来是看规格书，然后把规格书丢ai得到了。参考文献是在IEEE中写温度补偿的算法文章。
	
    return ((float)temp_raw / 326.8f) + 25.0f;
}

static void icm20602_quat_normalize(void)
{
    float norm;

    norm = sqrtf(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    if(0.000001f < norm)
    {
        q0 /= norm;
        q1 /= norm;
        q2 /= norm;
        q3 /= norm;
    }
    else
    {
        q0 = 1.0f;
        q1 = 0.0f;
        q2 = 0.0f;
        q3 = 0.0f;
    }
}

static void icm20602_set_quat_by_roll_pitch(float roll_rad, float pitch_rad)
{
    float cr;
    float sr;
    float cp;
    float sp;

    cr = cosf(roll_rad * 0.5f);
    sr = sinf(roll_rad * 0.5f);
    cp = cosf(pitch_rad * 0.5f);
    sp = sinf(pitch_rad * 0.5f);

    // yaw 默认为 0，只根据静止时重力方向初始化横滚/俯仰。
    q0 = cr * cp;
    q1 = sr * cp;
    q2 = cr * sp;
    q3 = -sr * sp;
    icm20602_quat_normalize();
}

static void icm20602_update_euler(void)
{
    float roll;
    float pitch;
    float yaw;
    float sinp;

    roll = atan2f(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2));
    sinp = 2.0f * (q0 * q2 - q3 * q1);
    sinp = icm20602_clamp_f(sinp, -1.0f, 1.0f);
    pitch = asinf(sinp);
    yaw = atan2f(2.0f * (q0 * q3 + q1 * q2), 1.0f - 2.0f * (q2 * q2 + q3 * q3));

    icm20602_filter_data.roll_mahony_deg = roll * 57.2957795f; //弧度转角度,180度转换的标志位
    icm20602_filter_data.pitch_mahony_deg = pitch * 57.2957795f;
    
	
	
	
	
	// 四元数Yaw只作为调试值保存。平装小车默认使用Z轴专用积分Yaw，避免Roll/Pitch融合过程对Yaw产生额外扰动。
	
	
	
	
	
    icm20602_filter_data.yaw_quat_deg = yaw * 57.2957795f;

#if (ICM20602_FUSION_ALGORITHM != ICM20602_FUSION_MAHONY_KALMAN)
    icm20602_filter_data.roll_deg = icm20602_filter_data.roll_mahony_deg;
    icm20602_filter_data.pitch_deg = icm20602_filter_data.pitch_mahony_deg;
#endif
}

static void icm20602_kalman_init(icm20602_kalman1d_t *kf, float init_angle)
{
    kf->angle = init_angle;
    kf->bias = 0.0f;
    kf->rate = 0.0f;
    kf->p00 = 1.0f;
    kf->p01 = 0.0f;
    kf->p10 = 0.0f;
    kf->p11 = 1.0f;

    // 这些是“小车场景”的保守初值。若车体震动大，优先增大测量噪声 R；若响应慢，适当减小 R 的方法增大 Q_angle。
    kf->q_angle = 0.006f;
    kf->q_bias = 0.0008f;
    kf->r_measure = 3.0f;
}

static float icm20602_kalman_update(icm20602_kalman1d_t *kf, float new_angle, float new_rate, float acc_weight, float dt_s)
{
    float r_dynamic;
    float y;
    float s;
    float k0;
    float k1;
    float p00_temp;
    float p01_temp;

    // 预测：角度由陀螺仪积分得到，bias 状态由测量残差慢慢估计。
    kf->rate = new_rate - kf->bias;
    kf->angle += dt_s * kf->rate;

    kf->p00 += dt_s * (dt_s * kf->p11 - kf->p01 - kf->p10 + kf->q_angle);
    kf->p01 -= dt_s * kf->p11;
    kf->p10 -= dt_s * kf->p11;
    kf->p11 += kf->q_bias * dt_s;

    // 更新：小车加减速或震动时，acc_weight 会变小，此时动态增大测量噪声 R，少信加速度角度。
    acc_weight = icm20602_clamp_f(acc_weight, 0.02f, 1.0f);
    r_dynamic = kf->r_measure / acc_weight;

    y = new_angle - kf->angle;
    if(y > 180.0f)
    {
        y -= 360.0f;
    }
    else if(y < -180.0f)
    {
        y += 360.0f;
    }

    s = kf->p00 + r_dynamic;
    if(0.000001f > s)
    {
        return kf->angle;
    }

    k0 = kf->p00 / s;
    k1 = kf->p10 / s;

    kf->angle += k0 * y;
    kf->bias += k1 * y;

    p00_temp = kf->p00;
    p01_temp = kf->p01;

    kf->p00 -= k0 * p00_temp;
    kf->p01 -= k0 * p01_temp;
    kf->p10 -= k1 * p00_temp;
    kf->p11 -= k1 * p01_temp;

    return kf->angle;
}

#if ICM20602_NOTCH_ENABLE
static void icm20602_biquad_notch_init(icm20602_biquad_t *bq, float sample_hz, float notch_hz, float q)
{
    float w0;
    float alpha;
    float cos_w0;
    float a0;

    notch_hz = icm20602_clamp_f(notch_hz, 1.0f, sample_hz * 0.45f);
    q = icm20602_clamp_f(q, 0.5f, 20.0f);

    w0 = 2.0f * M_PI * notch_hz / sample_hz;
    cos_w0 = cosf(w0);
    alpha = sinf(w0) / (2.0f * q);
    a0 = 1.0f + alpha;

    bq->b0 = 1.0f / a0;
    bq->b1 = -2.0f * cos_w0 / a0;
    bq->b2 = 1.0f / a0;
    bq->a1 = -2.0f * cos_w0 / a0;
    bq->a2 = (1.0f - alpha) / a0;
    bq->x1 = 0.0f;
    bq->x2 = 0.0f;
    bq->y1 = 0.0f;
    bq->y2 = 0.0f;
}

static float icm20602_biquad_update(icm20602_biquad_t *bq, float x)
{
    float y;

    y = bq->b0 * x + bq->b1 * bq->x1 + bq->b2 * bq->x2 - bq->a1 * bq->y1 - bq->a2 * bq->y2;
    bq->x2 = bq->x1;
    bq->x1 = x;
    bq->y2 = bq->y1;
    bq->y1 = y;
    return y;
}
#endif

static void icm20602_mahony_update(float gx_dps, float gy_dps, float gz_dps,
                                   float ax_g, float ay_g, float az_g,
                                   float acc_weight, float dt_s)
{
    float gx;
    float gy;
    float gz;
    float norm;
    float vx;
    float vy;
    float vz;
    float ex;
    float ey;
    float ez;
    float half_dt;
    float qa;
    float qb;
    float qc;
    float kp;
    float ki;

    gx = gx_dps * (M_PI / 180.0f);
    gy = gy_dps * (M_PI / 180.0f);
    gz = gz_dps * (M_PI / 180.0f);

    norm = sqrtf(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
    if((0.2f < norm) && (0.001f < acc_weight))
    {
        ax_g /= norm;
        ay_g /= norm;
        az_g /= norm;

        // 当前四元数估计出来的重力方向。
        vx = 2.0f * (q1 * q3 - q0 * q2);
        vy = 2.0f * (q0 * q1 + q2 * q3);
        vz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;

        // 测量重力方向与估计重力方向的叉乘误差。
        ex = (ay_g * vz - az_g * vy);
        ey = (az_g * vx - ax_g * vz);
        ez = (ax_g * vy - ay_g * vx);

        kp = ICM20602_MAHONY_KP * acc_weight;
        ki = ICM20602_MAHONY_KI * acc_weight;

        integral_x += ki * ex * dt_s;
        integral_y += ki * ey * dt_s;
        integral_z += ki * ez * dt_s;

        // 防止强震动后积分项积累过多，造成恢复慢。
        integral_x = icm20602_clamp_f(integral_x, -0.12f, 0.12f);
        integral_y = icm20602_clamp_f(integral_y, -0.12f, 0.12f);
        integral_z = icm20602_clamp_f(integral_z, -0.12f, 0.12f);

        gx += kp * ex + integral_x;
        gy += kp * ey + integral_y;
        gz += kp * ez + integral_z;
    }

    half_dt = 0.5f * dt_s;
    qa = q0;
    qb = q1;
    qc = q2;

    q0 += (-qb * gx - qc * gy - q3 * gz) * half_dt;
    q1 += ( qa * gx + qc * gz - q3 * gy) * half_dt;
    q2 += ( qa * gy - qb * gz + q3 * gx) * half_dt;
    q3 += ( qa * gz + qb * gy - qc * gx) * half_dt;

    icm20602_quat_normalize();
    icm20602_update_euler();
}

static void icm20602_madgwick_update(float gx_dps, float gy_dps, float gz_dps,
                                     float ax_g, float ay_g, float az_g,
                                     float acc_weight, float dt_s)
{
    float gx;
    float gy;
    float gz;
    float recip_norm;
    float s0;
    float s1;
    float s2;
    float s3;
    float q_dot0;
    float q_dot1;
    float q_dot2;
    float q_dot3;
    float _2q0;
    float _2q1;
    float _2q2;
    float _2q3;
    float _4q0;
    float _4q1;
    float _4q2;
    float _8q1;
    float _8q2;
    float q0q0;
    float q1q1;
    float q2q2;
    float q3q3;
    float beta;

    gx = gx_dps * (M_PI / 180.0f);
    gy = gy_dps * (M_PI / 180.0f);
    gz = gz_dps * (M_PI / 180.0f);

    q_dot0 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    q_dot1 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy);
    q_dot2 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx);
    q_dot3 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx);

    recip_norm = sqrtf(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
    if((0.2f < recip_norm) && (0.001f < acc_weight))
    {
        recip_norm = 1.0f / recip_norm;
        ax_g *= recip_norm;
        ay_g *= recip_norm;
        az_g *= recip_norm;

        _2q0 = 2.0f * q0;
        _2q1 = 2.0f * q1;
        _2q2 = 2.0f * q2;
        _2q3 = 2.0f * q3;
        _4q0 = 4.0f * q0;
        _4q1 = 4.0f * q1;
        _4q2 = 4.0f * q2;
        _8q1 = 8.0f * q1;
        _8q2 = 8.0f * q2;
        q0q0 = q0 * q0;
        q1q1 = q1 * q1;
        q2q2 = q2 * q2;
        q3q3 = q3 * q3;

        s0 = _4q0 * q2q2 + _2q2 * ax_g + _4q0 * q1q1 - _2q1 * ay_g;
        s1 = _4q1 * q3q3 - _2q3 * ax_g + 4.0f * q0q0 * q1 - _2q0 * ay_g - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az_g;
        s2 = 4.0f * q0q0 * q2 + _2q0 * ax_g + _4q2 * q3q3 - _2q3 * ay_g - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az_g;
        s3 = 4.0f * q1q1 * q3 - _2q1 * ax_g + 4.0f * q2q2 * q3 - _2q2 * ay_g;

        recip_norm = sqrtf(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
        if(0.000001f < recip_norm)
        {
            recip_norm = 1.0f / recip_norm;
            s0 *= recip_norm;
            s1 *= recip_norm;
            s2 *= recip_norm;
            s3 *= recip_norm;

            beta = ICM20602_MADGWICK_BETA * acc_weight;
            q_dot0 -= beta * s0;
            q_dot1 -= beta * s1;
            q_dot2 -= beta * s2;
            q_dot3 -= beta * s3;
        }
    }

    q0 += q_dot0 * dt_s;
    q1 += q_dot1 * dt_s;
    q2 += q_dot2 * dt_s;
    q3 += q_dot3 * dt_s;

    icm20602_quat_normalize();
    icm20602_update_euler();
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     ICM20602 滤波模块初始化
// 参数说明     void
// 返回参数     void
// 使用说明     在 icm20602_init() 成功后调用一次，用于清空滤波状态、四元数、卡尔曼状态和Yaw控制状态
// 注意事项     本函数只初始化软件滤波变量，不读取传感器；调用后还需要执行 icm20602_filter_calibrate()
//-------------------------------------------------------------------------------------------------------------------
void icm20602_filter_init(void)
{
    memset(&icm20602_filter_data, 0, sizeof(icm20602_filter_data));
    memset(&gyro_base_bias_dps, 0, sizeof(gyro_base_bias_dps));
    memset(&gyro_online_bias_dps, 0, sizeof(gyro_online_bias_dps));
    memset(&acc_lpf_g, 0, sizeof(acc_lpf_g));
    memset(&gyro_lpf_dps, 0, sizeof(gyro_lpf_dps));
    memset(acc_hist, 0, sizeof(acc_hist));
    memset(gyro_hist, 0, sizeof(gyro_hist));

    q0 = 1.0f;
    q1 = 0.0f;
    q2 = 0.0f;
    q3 = 0.0f;
    integral_x = 0.0f;
    integral_y = 0.0f;
    integral_z = 0.0f;
    last_acc_norm_g = 1.0f;
    static_count = 0;
    car_yaw_deg = 0.0f;
    yaw_no_turn_count = 0;
    yaw_turn_active = 0;
    yaw_turn_done = 0;
    yaw_turn_finish_count = 0;
    yaw_turn_start_deg = 0.0f;
    yaw_turn_target_deg = 0.0f;
    yaw_turn_error_deg = 0.0f;
    yaw_turn_output = 0.0f;
    hist_index = 0;
    hist_filled = 0;

    icm20602_kalman_init(&roll_kalman, 0.0f);
    icm20602_kalman_init(&pitch_kalman, 0.0f);

#if ICM20602_NOTCH_ENABLE
    icm20602_biquad_notch_init(&acc_notch[0], 1.0f / ICM20602_FILTER_DT, ICM20602_NOTCH_FREQ_HZ, ICM20602_NOTCH_Q);
    icm20602_biquad_notch_init(&acc_notch[1], 1.0f / ICM20602_FILTER_DT, ICM20602_NOTCH_FREQ_HZ, ICM20602_NOTCH_Q);
    icm20602_biquad_notch_init(&acc_notch[2], 1.0f / ICM20602_FILTER_DT, ICM20602_NOTCH_FREQ_HZ, ICM20602_NOTCH_Q);
    icm20602_biquad_notch_init(&gyro_notch[0], 1.0f / ICM20602_FILTER_DT, ICM20602_NOTCH_FREQ_HZ, ICM20602_NOTCH_Q);
    icm20602_biquad_notch_init(&gyro_notch[1], 1.0f / ICM20602_FILTER_DT, ICM20602_NOTCH_FREQ_HZ, ICM20602_NOTCH_Q);
    icm20602_biquad_notch_init(&gyro_notch[2], 1.0f / ICM20602_FILTER_DT, ICM20602_NOTCH_FREQ_HZ, ICM20602_NOTCH_Q);
#endif

    icm20602_filter_data.fusion_mode = ICM20602_FUSION_ALGORITHM;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     ICM20602 上电静止校准函数
// 参数说明     sample_count        校准采样次数，推荐 800 次；采样间隔约 5ms，约等于静止 4 秒
// 返回参数     uint8               0 表示校准完成
// 使用说明     小车上电后、定时器开始滤波前调用；校准期间车体必须保持静止，不能拿手晃动
// 主要作用     计算三轴陀螺仪零偏、温度基准、初始 Roll/Pitch，并把 Yaw 清零
//-------------------------------------------------------------------------------------------------------------------
uint8 icm20602_filter_calibrate(uint16 sample_count)
{
    uint16 i;
    float sum_ax;
    float sum_ay;
    float sum_az;
    float sum_gx;
    float sum_gy;
    float sum_gz;
    float sum_temp;
    float mean_ax;
    float mean_ay;
    float mean_az;
    float roll;
    float pitch;
    float roll_deg;
    float pitch_deg;

    if(100U > sample_count)
    {
        sample_count = 100U;
    }

    sum_ax = 0.0f;
    sum_ay = 0.0f;
    sum_az = 0.0f;
    sum_gx = 0.0f;
    sum_gy = 0.0f;
    sum_gz = 0.0f;
    sum_temp = 0.0f;

    // 上电校准时务必让小车静止。这里直接求均值，得到最重要的陀螺仪零偏。
    for(i = 0; i < sample_count; i ++)
    {
        icm20602_get_acc_gyro();
        sum_ax += icm20602_acc_transition(icm20602_acc_x);
        sum_ay += icm20602_acc_transition(icm20602_acc_y);
        sum_az += icm20602_acc_transition(icm20602_acc_z);
        sum_gx += icm20602_gyro_transition(icm20602_gyro_x);
        sum_gy += icm20602_gyro_transition(icm20602_gyro_y);
        sum_gz += icm20602_gyro_transition(icm20602_gyro_z);
        sum_temp += icm20602_temp_to_c(icm20602_temp_raw);
        system_delay_ms(ICM20602_CALIBRATE_DELAY_MS);
    }

    mean_ax = sum_ax / (float)sample_count;
    mean_ay = sum_ay / (float)sample_count;
    mean_az = sum_az / (float)sample_count;

    gyro_base_bias_dps.x = sum_gx / (float)sample_count;
    gyro_base_bias_dps.y = sum_gy / (float)sample_count;
    gyro_base_bias_dps.z = sum_gz / (float)sample_count;
    gyro_online_bias_dps.x = 0.0f;
    gyro_online_bias_dps.y = 0.0f;
    gyro_online_bias_dps.z = 0.0f;
    temperature_base_c = sum_temp / (float)sample_count;

    acc_lpf_g.x = mean_ax;
    acc_lpf_g.y = mean_ay;
    acc_lpf_g.z = mean_az;
    gyro_lpf_dps.x = 0.0f;
    gyro_lpf_dps.y = 0.0f;
    gyro_lpf_dps.z = 0.0f;
    last_acc_norm_g = sqrtf(mean_ax * mean_ax + mean_ay * mean_ay + mean_az * mean_az);

    roll = atan2f(mean_ay, mean_az);
    pitch = atan2f(-mean_ax, sqrtf(mean_ay * mean_ay + mean_az * mean_az));
    roll_deg = roll * 57.2957795f;
    pitch_deg = pitch * 57.2957795f;

    icm20602_set_quat_by_roll_pitch(roll, pitch);
    icm20602_update_euler();
    icm20602_kalman_init(&roll_kalman, roll_deg);
    icm20602_kalman_init(&pitch_kalman, pitch_deg);

    icm20602_filter_data.roll_acc_deg = roll_deg;
    icm20602_filter_data.pitch_acc_deg = pitch_deg;
    icm20602_filter_data.roll_kalman_deg = roll_deg;
    icm20602_filter_data.pitch_kalman_deg = pitch_deg;
    icm20602_filter_data.roll_deg = roll_deg;
    icm20602_filter_data.pitch_deg = pitch_deg;
    car_yaw_deg = 0.0f;
    icm20602_filter_data.yaw_deg = 0.0f;
    icm20602_filter_data.yaw_quat_deg = 0.0f;
    icm20602_filter_data.yaw_rate_dps = 0.0f;
    icm20602_filter_data.yaw_bias_dps = gyro_online_bias_dps.z;
    icm20602_filter_data.calibrated = 1;
    icm20602_filter_data.temperature_c = temperature_base_c;
    icm20602_filter_data.acc_weight = 1.0f;
    icm20602_filter_data.turn_active = 0;
    icm20602_filter_data.turn_done = 0;
    icm20602_filter_data.turn_target_deg = 0.0f;
    icm20602_filter_data.turn_error_deg = 0.0f;
    icm20602_filter_data.turn_output = 0.0f;

    return 0;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     ICM20602 滤波与姿态更新函数
// 参数说明     dt_s                两次调用之间的时间间隔，单位 s；本工程固定传入 ICM20602_FILTER_DT = 0.005f
// 返回参数     void
// 使用说明     产品使用时建议放在 5ms 定时器中调用，不能被 IPS200 刷屏、串口打印或电机控制逻辑拖慢
// 输出结果     更新 icm20602_filter_data 结构体，车跑起来时直接读取该结构体中的 roll/pitch/yaw/gz 等滤波数据
//-------------------------------------------------------------------------------------------------------------------
void icm20602_filter_update(float dt_s)
{
    int16 ax_raw;
    int16 ay_raw;
    int16 az_raw;
    int16 gx_raw;
    int16 gy_raw;
    int16 gz_raw;
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
    float temp_c;
    float acc_norm;
    float acc_error;
    float acc_jerk;
    float acc_alpha;
    float gyro_alpha;
    float acc_weight;
    float gyro_abs_sum;
    float gyro_abs_max;
    float temperature_delta;
    float yaw_rate_for_integral;
    float yaw_deadband_now;
    uint8 yaw_is_turning_now;
    uint8 yaw_allow_bias_learn;
    float yaw_no_turn_limit;
    float roll_acc;
    float pitch_acc;
    float roll_rate;
    float pitch_rate;
    float roll_rad;
    float pitch_rad;
    float cos_roll;
    float sin_roll;
    float tan_pitch;
    icm20602_vec3f_t gyro_temp_bias;

    if(0.0005f > dt_s)
    {
        dt_s = ICM20602_FILTER_DT;
    }

    icm20602_get_acc_gyro();

    acc_hist[hist_index][0] = icm20602_acc_x;
    acc_hist[hist_index][1] = icm20602_acc_y;
    acc_hist[hist_index][2] = icm20602_acc_z;
    gyro_hist[hist_index][0] = icm20602_gyro_x;
    gyro_hist[hist_index][1] = icm20602_gyro_y;
    gyro_hist[hist_index][2] = icm20602_gyro_z;

    hist_index ++;
    if(3U <= hist_index)
    {
        hist_index = 0;
        hist_filled = 1;
    }

    if(hist_filled)
    {
        ax_raw = icm20602_median3_int16(acc_hist[0][0], acc_hist[1][0], acc_hist[2][0]);
        ay_raw = icm20602_median3_int16(acc_hist[0][1], acc_hist[1][1], acc_hist[2][1]);
        az_raw = icm20602_median3_int16(acc_hist[0][2], acc_hist[1][2], acc_hist[2][2]);
        gx_raw = icm20602_median3_int16(gyro_hist[0][0], gyro_hist[1][0], gyro_hist[2][0]);
        gy_raw = icm20602_median3_int16(gyro_hist[0][1], gyro_hist[1][1], gyro_hist[2][1]);
        gz_raw = icm20602_median3_int16(gyro_hist[0][2], gyro_hist[1][2], gyro_hist[2][2]);
    }
    else
    {
        ax_raw = icm20602_acc_x;
        ay_raw = icm20602_acc_y;
        az_raw = icm20602_acc_z;
        gx_raw = icm20602_gyro_x;
        gy_raw = icm20602_gyro_y;
        gz_raw = icm20602_gyro_z;
    }

    ax = icm20602_acc_transition(ax_raw);
    ay = icm20602_acc_transition(ay_raw);
    az = icm20602_acc_transition(az_raw);
    gx = icm20602_gyro_transition(gx_raw);
    gy = icm20602_gyro_transition(gy_raw);
    gz = icm20602_gyro_transition(gz_raw);
    temp_c = icm20602_temp_to_c(icm20602_temp_raw);

    temperature_delta = temp_c - temperature_base_c;
    gyro_temp_bias.x = gyro_temp_k_dps_per_c.x * temperature_delta;
    gyro_temp_bias.y = gyro_temp_k_dps_per_c.y * temperature_delta;
    gyro_temp_bias.z = gyro_temp_k_dps_per_c.z * temperature_delta;

    // 陀螺仪：先扣除上电零偏、温漂补偿和在线零偏，再进入后续滤波。
    gx = gx - gyro_base_bias_dps.x - gyro_temp_bias.x - gyro_online_bias_dps.x;
    gy = gy - gyro_base_bias_dps.y - gyro_temp_bias.y - gyro_online_bias_dps.y;
    gz = gz - gyro_base_bias_dps.z - gyro_temp_bias.z - gyro_online_bias_dps.z;

#if ICM20602_NOTCH_ENABLE
    ax = icm20602_biquad_update(&acc_notch[0], ax);
    ay = icm20602_biquad_update(&acc_notch[1], ay);
    az = icm20602_biquad_update(&acc_notch[2], az);
    gx = icm20602_biquad_update(&gyro_notch[0], gx);
    gy = icm20602_biquad_update(&gyro_notch[1], gy);
    gz = icm20602_biquad_update(&gyro_notch[2], gz);
#endif

    acc_norm = sqrtf(ax * ax + ay * ay + az * az);
    acc_error = icm20602_abs_f(acc_norm - 1.0f);
    acc_jerk = icm20602_abs_f(acc_norm - last_acc_norm_g);
    last_acc_norm_g = acc_norm;

    gyro_abs_max = icm20602_abs_f(gx);
    if(gyro_abs_max < icm20602_abs_f(gy))
    {
        gyro_abs_max = icm20602_abs_f(gy);
    }
    if(gyro_abs_max < icm20602_abs_f(gz))
    {
        gyro_abs_max = icm20602_abs_f(gz);
    }

    if((ICM20602_VIB_ACC_LIMIT_G < acc_error) || (ICM20602_VIB_JERK_LIMIT_G < acc_jerk) || (ICM20602_VIB_GYRO_LIMIT_DPS < gyro_abs_max))
    {
        icm20602_filter_data.vibration_flag = 1;
        acc_alpha = icm20602_lpf_alpha(ICM20602_VIB_ACC_CUTOFF_HZ, dt_s);
        gyro_alpha = icm20602_lpf_alpha(ICM20602_VIB_GYRO_CUTOFF_HZ, dt_s);
    }
    else
    {
        icm20602_filter_data.vibration_flag = 0;
        acc_alpha = icm20602_lpf_alpha(ICM20602_NORMAL_ACC_CUTOFF_HZ, dt_s);
        gyro_alpha = icm20602_lpf_alpha(ICM20602_NORMAL_GYRO_CUTOFF_HZ, dt_s);
    }

    acc_lpf_g.x += acc_alpha * (ax - acc_lpf_g.x);
    acc_lpf_g.y += acc_alpha * (ay - acc_lpf_g.y);
    acc_lpf_g.z += acc_alpha * (az - acc_lpf_g.z);
    gyro_lpf_dps.x += gyro_alpha * (gx - gyro_lpf_dps.x);
    gyro_lpf_dps.y += gyro_alpha * (gy - gyro_lpf_dps.y);
    gyro_lpf_dps.z += gyro_alpha * (gz - gyro_lpf_dps.z);

    gyro_abs_sum = icm20602_abs_f(gyro_lpf_dps.x) + icm20602_abs_f(gyro_lpf_dps.y) + icm20602_abs_f(gyro_lpf_dps.z);

    if((ICM20602_STATIC_GYRO_LIMIT_DPS > gyro_abs_sum) && (ICM20602_STATIC_ACC_LIMIT_G > acc_error) && (0U == icm20602_filter_data.vibration_flag))
    {
        icm20602_filter_data.static_flag = 1;
        if(static_count < 60000U)
        {
            static_count ++;
        }

        // 小车确实静止时，慢速学习残余零偏。这样温度慢慢变化时，零偏会自动被拉回。
        // 注意只在静止状态学习，避免车辆转弯时把真实角速度误认为零偏。
        gyro_online_bias_dps.x += ICM20602_ONLINE_BIAS_ALPHA * gx;
        gyro_online_bias_dps.y += ICM20602_ONLINE_BIAS_ALPHA * gy;
        gyro_online_bias_dps.z += ICM20602_ONLINE_BIAS_ALPHA * gz;

        // ZARU 零角速度更新：静止时把残余角速度继续压向 0，降低姿态积分漂移。
        if(8U < static_count)
        {
            gyro_lpf_dps.x *= (1.0f - ICM20602_STATIC_GYRO_DAMP_ALPHA);
            gyro_lpf_dps.y *= (1.0f - ICM20602_STATIC_GYRO_DAMP_ALPHA);
            gyro_lpf_dps.z *= (1.0f - ICM20602_STATIC_GYRO_DAMP_ALPHA);
        }
    }
    else
    {
        icm20602_filter_data.static_flag = 0;
        static_count = 0;
    }

    // 加速度可信度自适应：震动/加减速越强，越少用加速度修正姿态，避免小车震动把角度带偏。
    acc_weight = 1.0f - (acc_error / 0.25f);
    acc_weight -= (acc_jerk / 0.40f);
    if(1U == icm20602_filter_data.vibration_flag)
    {
        acc_weight *= 0.30f;
    }
    if(1U == icm20602_filter_data.static_flag)
    {
        acc_weight = 1.0f;
    }
    acc_weight = icm20602_clamp_f(acc_weight, 0.0f, 1.0f);

    //===================================== 平装小车专用 Yaw 优化 =====================================
    // 关键修改：转向过程中使用“小死区 + 禁止零偏学习”，避免转90度时尾段被死区/静止锁定吃掉。
    // 非转向状态仍使用较大死区和静止锁定，保证停车不乱漂。
    yaw_is_turning_now = 0;
    if((1U == yaw_turn_active) || (ICM20602_YAW_TURN_DYNAMIC_DPS < icm20602_abs_f(gyro_lpf_dps.z)))
    {
        yaw_is_turning_now = 1;
    }

    yaw_deadband_now = yaw_is_turning_now ? ICM20602_YAW_TURN_DEADBAND_DPS : yaw_deadband_dps;
    yaw_allow_bias_learn = yaw_is_turning_now ? 0U : 1U;

    yaw_no_turn_limit = yaw_deadband_dps + 0.55f;
    if(yaw_no_turn_limit < 0.8f)
    {
        yaw_no_turn_limit = 0.8f;
    }

    if((1U == yaw_allow_bias_learn)
    && (icm20602_abs_f(gyro_lpf_dps.z) < yaw_no_turn_limit)
    && (icm20602_abs_f(gyro_lpf_dps.x) < ICM20602_YAW_NO_TURN_GYRO_XY_DPS)
    && (icm20602_abs_f(gyro_lpf_dps.y) < ICM20602_YAW_NO_TURN_GYRO_XY_DPS)
    && (acc_error < ICM20602_YAW_NO_TURN_ACC_ERR_G))
    {
        if(yaw_no_turn_count < 60000U)
        {
            yaw_no_turn_count ++;
        }

        // 只有确认不是转向时才学习Z轴零偏。转90度时绝对不能学习，否则会把真实角速度吃掉。
        if(yaw_no_turn_count > ICM20602_YAW_NO_TURN_CONFIRM_COUNT)
        {
            gyro_online_bias_dps.z += yaw_bias_alpha * gyro_lpf_dps.z;
            gyro_online_bias_dps.z = icm20602_clamp_f(gyro_online_bias_dps.z,
                                                      -ICM20602_YAW_BIAS_LIMIT_DPS,
                                                       ICM20602_YAW_BIAS_LIMIT_DPS);
        }
    }
    else
    {
        yaw_no_turn_count = 0;
    }

    if((1U == yaw_hold_enable) && (1U == icm20602_filter_data.static_flag) && (0U == yaw_is_turning_now))
    {
        yaw_rate_for_integral = 0.0f;
    }
    else
    {
        yaw_rate_for_integral = icm20602_apply_deadband_f(gyro_lpf_dps.z, yaw_deadband_now);
    }

    car_yaw_deg += yaw_rate_for_integral * dt_s * yaw_scale;
    if(car_yaw_deg > 3600.0f)
    {
        car_yaw_deg -= 3600.0f;
    }
    else if(car_yaw_deg < -3600.0f)
    {
        car_yaw_deg += 3600.0f;
    }
    //=================================================================================================

#if (ICM20602_FUSION_ALGORITHM == ICM20602_FUSION_MADGWICK)
    icm20602_madgwick_update(gyro_lpf_dps.x, gyro_lpf_dps.y, gyro_lpf_dps.z,
                             acc_lpf_g.x, acc_lpf_g.y, acc_lpf_g.z,
                             acc_weight, dt_s);
#else
    icm20602_mahony_update(gyro_lpf_dps.x, gyro_lpf_dps.y, gyro_lpf_dps.z,
                           acc_lpf_g.x, acc_lpf_g.y, acc_lpf_g.z,
                           acc_weight, dt_s);
#endif

    // 一维 Kalman：用陀螺预测，用加速度角度更新。小车主要关心 Roll/Pitch 稳定，默认用它作为最终输出。
    roll_acc = atan2f(acc_lpf_g.y, acc_lpf_g.z) * 57.2957795f;
    pitch_acc = atan2f(-acc_lpf_g.x, sqrtf(acc_lpf_g.y * acc_lpf_g.y + acc_lpf_g.z * acc_lpf_g.z)) * 57.2957795f;

    roll_rad = roll_kalman.angle * (M_PI / 180.0f);
    pitch_rad = pitch_kalman.angle * (M_PI / 180.0f);
    cos_roll = cosf(roll_rad);
    sin_roll = sinf(roll_rad);
    pitch_rad = icm20602_clamp_f(pitch_rad, -1.45f, 1.45f);                    // 避免 tan 接近无穷大
    tan_pitch = tanf(pitch_rad);

    roll_rate = gyro_lpf_dps.x + sin_roll * tan_pitch * gyro_lpf_dps.y + cos_roll * tan_pitch * gyro_lpf_dps.z;
    pitch_rate = cos_roll * gyro_lpf_dps.y - sin_roll * gyro_lpf_dps.z;

    icm20602_filter_data.roll_kalman_deg = icm20602_kalman_update(&roll_kalman, roll_acc, roll_rate, acc_weight, dt_s);
    icm20602_filter_data.pitch_kalman_deg = icm20602_kalman_update(&pitch_kalman, pitch_acc, pitch_rate, acc_weight, dt_s);

#if (ICM20602_FUSION_ALGORITHM == ICM20602_FUSION_MAHONY_KALMAN)
    icm20602_filter_data.roll_deg = icm20602_filter_data.roll_kalman_deg;
    icm20602_filter_data.pitch_deg = icm20602_filter_data.pitch_kalman_deg;
#endif

    icm20602_filter_data.yaw_deg = car_yaw_deg;
    icm20602_filter_data.yaw_rate_dps = yaw_rate_for_integral;
    icm20602_filter_data.yaw_bias_dps = gyro_online_bias_dps.z;
    icm20602_filter_data.turn_active = yaw_turn_active;
    icm20602_filter_data.turn_done = yaw_turn_done;
    icm20602_filter_data.turn_target_deg = yaw_turn_target_deg - yaw_turn_start_deg;
    icm20602_filter_data.turn_error_deg = yaw_turn_error_deg;
    icm20602_filter_data.turn_output = yaw_turn_output;
    icm20602_filter_data.roll_acc_deg = roll_acc;
    icm20602_filter_data.pitch_acc_deg = pitch_acc;
    icm20602_filter_data.ax_g = acc_lpf_g.x;
    icm20602_filter_data.ay_g = acc_lpf_g.y;
    icm20602_filter_data.az_g = acc_lpf_g.z;
    icm20602_filter_data.gx_dps = gyro_lpf_dps.x;
    icm20602_filter_data.gy_dps = gyro_lpf_dps.y;
    icm20602_filter_data.gz_dps = gyro_lpf_dps.z;
    icm20602_filter_data.temperature_c = temp_c;
    icm20602_filter_data.acc_norm_g = acc_norm;
    icm20602_filter_data.acc_weight = acc_weight;
    icm20602_filter_data.fusion_mode = ICM20602_FUSION_ALGORITHM;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     Yaw 航向角清零函数
// 参数说明     void
// 返回参数     void
// 使用说明     开始转 90 度、开始跑固定路线、重新设定当前车头方向为 0 度时调用
// 注意事项     只清零 Yaw 和转角控制状态，Roll/Pitch 会保留当前姿态，不影响车身倾斜判断
//-------------------------------------------------------------------------------------------------------------------
void icm20602_filter_reset_yaw(void)
{
    float roll;
    float pitch;

    // 保留当前 Roll/Pitch，单独把 Yaw 归零。小车启动前或需要重新定航向时可调用。
    roll = icm20602_filter_data.roll_deg * (M_PI / 180.0f);
    pitch = icm20602_filter_data.pitch_deg * (M_PI / 180.0f);
    q0 = 1.0f;
    q1 = 0.0f;
    q2 = 0.0f;
    q3 = 0.0f;
    icm20602_set_quat_by_roll_pitch(roll, pitch);
    integral_x = 0.0f;
    integral_y = 0.0f;
    integral_z = 0.0f;
    car_yaw_deg = 0.0f;
    yaw_turn_active = 0;
    yaw_turn_done = 0;
    yaw_turn_finish_count = 0;
    yaw_turn_start_deg = 0.0f;
    yaw_turn_target_deg = 0.0f;
    yaw_turn_error_deg = 0.0f;
    yaw_turn_output = 0.0f;
    icm20602_filter_data.yaw_deg = 0.0f;
    icm20602_filter_data.yaw_rate_dps = 0.0f;
    icm20602_filter_data.turn_active = 0;
    icm20602_filter_data.turn_done = 0;
    icm20602_filter_data.turn_target_deg = 0.0f;
    icm20602_filter_data.turn_error_deg = 0.0f;
    icm20602_filter_data.turn_output = 0.0f;
    icm20602_update_euler();
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     设置陀螺仪温漂补偿系数
// 参数说明     gx_k                X轴温漂系数，单位 °/s/℃
// 参数说明     gy_k                Y轴温漂系数，单位 °/s/℃
// 参数说明     gz_k                Z轴温漂系数，单位 °/s/℃
// 返回参数     void
// 使用说明     需要先采集不同温度下的静止陀螺仪数据并线性拟合；没有实测数据时保持默认 0 即可
//-------------------------------------------------------------------------------------------------------------------
void icm20602_filter_set_gyro_temp_coeff(float gx_k, float gy_k, float gz_k)
{
    // 温漂系数不是通用常数，需要同一块板子在不同温度、静止状态下采集数据后线性拟合。
    // 若暂时没有温箱/热风枪测试数据，保持 0 即可，静止在线零偏学习会补偿一部分慢温漂。
    gyro_temp_k_dps_per_c.x = gx_k;
    gyro_temp_k_dps_per_c.y = gy_k;
    gyro_temp_k_dps_per_c.z = gz_k;
}


//-------------------------------------------------------------------------------------------------------------------
// 函数简介     设置 Yaw 角度比例修正系数
// 参数说明     scale               比例系数，例如真实转 90 度，显示 86 度，则传入 90.0f / 86.0f
// 返回参数     void
// 使用说明     用于修正固定比例误差；不要用它强行修正随机漂移，漂移应优先通过校准、死区和外部传感器解决
//-------------------------------------------------------------------------------------------------------------------
void icm20602_filter_set_yaw_scale(float scale)
{
    // 用实测比例修正固定误差。例：真实90度，显示86度，则 scale=90/86。
    // 不建议用很夸张的比例修正长期漂移；长期漂移主要靠零偏、死区和外部参考解决。
    if(scale < 0.20f)
    {
        scale = 0.20f;
    }
    else if(scale > 5.00f)
    {
        scale = 5.00f;
    }
    yaw_scale = scale;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     设置平装小车 Yaw 抗漂参数
// 参数说明     deadband_dps        Z轴角速度死区，单位 °/s；漂移大可适当加大，慢速转弯被吃掉则减小
// 参数说明     bias_alpha          直行/静止时 Z 轴在线零偏学习速度，越大修正越快，但误学习风险越高
// 参数说明     hold_enable         静止时是否锁住 Yaw，1 开启，0 关闭，普通小车推荐开启
// 返回参数     void
// 使用说明     调试时可在 main 中校准完成后调用，例如 icm20602_filter_set_yaw_opt(0.80f, 0.00060f, 1)
//-------------------------------------------------------------------------------------------------------------------
void icm20602_filter_set_yaw_opt(float deadband_dps, float bias_alpha, uint8 hold_enable)
{
    // 小车调参建议：
    // 漂移大：deadband 从0.65逐步加到0.8/1.0。
    // 慢速转向被吃掉：deadband 从0.65降到0.4/0.3。
    // 停车温漂明显：bias_alpha 从0.00045加到0.0008，但过大会误学真实转向。
    yaw_deadband_dps = icm20602_clamp_f(deadband_dps, 0.0f, 3.0f);
    yaw_bias_alpha = icm20602_clamp_f(bias_alpha, 0.0f, 0.01f);
    yaw_hold_enable = hold_enable ? 1U : 0U;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     清除运行中学习到的在线零偏
// 参数说明     void
// 返回参数     void
// 使用说明     调试 Yaw 漂移、修改死区参数、或者发现误学习后可调用；不会清除上电静止校准得到的基础零偏
//-------------------------------------------------------------------------------------------------------------------
void icm20602_filter_clear_online_bias(void)
{
    gyro_online_bias_dps.x = 0.0f;
    gyro_online_bias_dps.y = 0.0f;
    gyro_online_bias_dps.z = 0.0f;
    yaw_no_turn_count = 0;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     启动小车目标转角控制
// 参数说明     target_deg          目标相对转角，单位 °；例如 90.0f 表示左/右转90度，-90.0f 表示反方向90度
// 返回参数     void
// 使用说明     调用后主循环中反复调用 icm20602_filter_yaw_turn_update()，把返回值映射到左右轮差速
// 注意事项     本函数不会直接控制电机，只负责根据IMU计算目标角度误差和建议转向输出
//-------------------------------------------------------------------------------------------------------------------
void icm20602_filter_yaw_turn_start(float target_deg)
{
    // 目标相对角度：正数和负数分别代表两个旋转方向。
    // 调用本函数后，Yaw不强制清零，而是记录起点，这样转向前的累计Yaw不会影响控制。
    target_deg = icm20602_clamp_f(target_deg, -720.0f, 720.0f);

    yaw_turn_start_deg = car_yaw_deg;
    yaw_turn_target_deg = yaw_turn_start_deg + target_deg;
    yaw_turn_error_deg = target_deg;
    yaw_turn_output = 0.0f;
    yaw_turn_finish_count = 0;
    yaw_turn_done = 0;
    yaw_turn_active = 1;
    yaw_no_turn_count = 0;

    icm20602_filter_data.turn_active = 1;
    icm20602_filter_data.turn_done = 0;
    icm20602_filter_data.turn_target_deg = target_deg;
    icm20602_filter_data.turn_error_deg = yaw_turn_error_deg;
    icm20602_filter_data.turn_output = 0.0f;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     更新目标转角闭环并返回转向输出
// 参数说明     void
// 返回参数     float               转向输出，范围 -1.0f ~ 1.0f；正负表示两个旋转方向
// 使用说明     在电机控制循环中周期调用，把返回值乘以最大PWM后给左右轮差速
// 示例说明     left_pwm = -turn * TURN_PWM_MAX；right_pwm = turn * TURN_PWM_MAX；若方向反了，交换正负号
//-------------------------------------------------------------------------------------------------------------------
float icm20602_filter_yaw_turn_update(void)
{
    float error;
    float output;
    float sign;

    if(0U == yaw_turn_active)
    {
        yaw_turn_output = 0.0f;
        icm20602_filter_data.turn_output = 0.0f;
        return 0.0f;
    }

    error = icm20602_turn_error_f(yaw_turn_target_deg, car_yaw_deg);
    yaw_turn_error_deg = error;

    // PD闭环：P负责接近目标，D按当前角速度提前刹车，减少“到90再停导致过冲”。
    // 注意 yaw_rate_dps 带方向，和 error 同号表示仍在向目标方向转。
    output = yaw_turn_kp * error - yaw_turn_kd * icm20602_filter_data.yaw_rate_dps;
    output = icm20602_clamp_f(output, -yaw_turn_max_output, yaw_turn_max_output);

    // 电机有死区。误差还没到位时，给一个最小输出；到位附近不强行补，避免来回抖。
    if(icm20602_abs_f(error) > yaw_turn_finish_error_deg)
    {
        sign = icm20602_sign_f(output);
        if(0.0f == sign)
        {
            sign = icm20602_sign_f(error);
        }
        if(icm20602_abs_f(output) < yaw_turn_min_output)
        {
            output = sign * yaw_turn_min_output;
        }
    }

    if((icm20602_abs_f(error) <= yaw_turn_finish_error_deg)
    && (icm20602_abs_f(icm20602_filter_data.yaw_rate_dps) <= yaw_turn_finish_rate_dps))
    {
        if(yaw_turn_finish_count < 60000U)
        {
            yaw_turn_finish_count ++;
        }
    }
    else
    {
        yaw_turn_finish_count = 0;
    }

    if(yaw_turn_finish_count >= ICM20602_YAW_TURN_FINISH_COUNT)
    {
        yaw_turn_active = 0;
        yaw_turn_done = 1;
        yaw_turn_output = 0.0f;
        icm20602_filter_data.turn_active = 0;
        icm20602_filter_data.turn_done = 1;
        icm20602_filter_data.turn_error_deg = error;
        icm20602_filter_data.turn_output = 0.0f;
        return 0.0f;
    }

    yaw_turn_output = output;
    icm20602_filter_data.turn_active = 1;
    icm20602_filter_data.turn_done = 0;
    icm20602_filter_data.turn_error_deg = error;
    icm20602_filter_data.turn_output = output;
    return output;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     查询目标转角是否完成
// 参数说明     void
// 返回参数     uint8               1 表示已经到位，0 表示还在转向
// 使用说明     小车转90度时，while循环里检测该函数；返回1后停止左右电机
//-------------------------------------------------------------------------------------------------------------------
uint8 icm20602_filter_yaw_turn_is_done(void)
{
    return yaw_turn_done;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     取消当前目标转角控制
// 参数说明     void
// 返回参数     void
// 使用说明     遇到急停、遥控接管、避障触发或重新规划路线时调用，调用后 turn_output 会归零
//-------------------------------------------------------------------------------------------------------------------
void icm20602_filter_yaw_turn_cancel(void)
{
    yaw_turn_active = 0;
    yaw_turn_done = 0;
    yaw_turn_finish_count = 0;
    yaw_turn_error_deg = 0.0f;
    yaw_turn_output = 0.0f;

    icm20602_filter_data.turn_active = 0;
    icm20602_filter_data.turn_done = 0;
    icm20602_filter_data.turn_error_deg = 0.0f;
    icm20602_filter_data.turn_output = 0.0f;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     设置目标转角闭环控制参数
// 参数说明     kp                  角度误差比例系数，越大转得越猛
// 参数说明     kd                  角速度刹车系数，越大越早减速，过冲时可适当增大
// 参数说明     max_output          最大转向输出，范围 0.10~1.00
// 参数说明     min_output          最小转向输出，用于克服电机死区
// 参数说明     finish_error_deg    到位角度误差阈值，单位 °
// 参数说明     finish_rate_dps     到位时允许的最大角速度，单位 °/s
// 返回参数     void
// 使用说明     转过头就减小 kp 或增大 kd；转不到就增大 kp/min_output 或减小 kd
//-------------------------------------------------------------------------------------------------------------------
void icm20602_filter_yaw_turn_set_param(float kp, float kd, float max_output, float min_output, float finish_error_deg, float finish_rate_dps)
{
    yaw_turn_kp = icm20602_clamp_f(kp, 0.001f, 0.200f);
    yaw_turn_kd = icm20602_clamp_f(kd, 0.000f, 0.100f);
    yaw_turn_max_output = icm20602_clamp_f(max_output, 0.10f, 1.00f);
    yaw_turn_min_output = icm20602_clamp_f(min_output, 0.00f, yaw_turn_max_output);
    yaw_turn_finish_error_deg = icm20602_clamp_f(finish_error_deg, 0.3f, 10.0f);
    yaw_turn_finish_rate_dps = icm20602_clamp_f(finish_rate_dps, 1.0f, 50.0f);
}

