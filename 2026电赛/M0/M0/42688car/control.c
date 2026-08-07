#include "control.h"

#include "app_config.h"
#include "encoder.h"
#include "icm42688.h"
#include "motor.h"
#include "pid.h"

typedef struct {
    int16_t left_speed;
    int16_t right_speed;
    bool angle_loop_enable;
    float target_angle_deg;
} CarCommand_t;

/* 左右轮速度环、车头角度环，各自独立保存 PID 状态。 */
static PIDController_t g_left_speed_pid;
static PIDController_t g_right_speed_pid;
static PIDController_t g_angle_pid;

/* g_command 是外部命令缓存，Car_MotorControl() 只改这里。 */
static volatile CarCommand_t g_command;

/* g_state 是调试状态缓存，OLED 和串口调试都可以读取。 */
static volatile CarControlState_t g_state;

static void reset_speed_loop(void)
{
    PID_Reset(&g_left_speed_pid);
    PID_Reset(&g_right_speed_pid);
}

void Car_ControlInit(void)
{
    /*
     * 初始化三个 PID，参数都集中放在 app_config.h，方便调车时统一修改：
     *
     * 左轮速度环：CAR_SPEED_KP=50，CAR_SPEED_KI=18，CAR_SPEED_KD=0。
     * 右轮速度环：和左轮使用同一组参数，先保证两边调车逻辑一致。
     * 车头角度环：CAR_ANGLE_KP=0.39，CAR_ANGLE_KI=0.01，CAR_ANGLE_KD=0.03。
     *
     * 速度环输出直接就是左右轮 PWM。
     * 角度环输出是左右轮差速修正量。
     */
    SpeedPID_Init(&g_left_speed_pid);
    SpeedPID_Init(&g_right_speed_pid);
    AnglePID_Init(&g_angle_pid);

    Motor_Init();
    Encoder_Init();
    Odometry_Reset(&g_carOdometry);
    Car_MotorControl(0, 0, false, 0.0f);
}

void Car_MotorControl(int16_t left_speed, int16_t right_speed,
                      bool angle_loop_enable, float target_angle_deg)
{
    /*
     * 对外唯一运动控制入口：
     * left_speed/right_speed 是左右轮目标速度，单位为 10ms 内编码器 tick。
     * angle_loop_enable=true 时，角度环会在左右目标速度上叠加差速修正。
     */
    g_command.left_speed = left_speed;
    g_command.right_speed = right_speed;
    g_command.angle_loop_enable = angle_loop_enable;
    g_command.target_angle_deg = PID_WrapDegrees(target_angle_deg);
}

void Car_Stop(void)
{
    Car_MotorControl(0, 0, false, 0.0f);
    reset_speed_loop();
    PID_Reset(&g_angle_pid);
    Motor_Stop();
}

void Car_ControlUpdate10ms(void)
{
    EncoderState_t encoder;
    ICM42688_Data_t imu;
    CarCommand_t command;
    int16_t angle_correction = 0;
    int16_t left_target;
    int16_t right_target;
    int16_t left_pwm;
    int16_t right_pwm;
    bool imu_ready;

    /* 先复制命令，避免中断执行过程中外部又改 g_command。 */
    command = g_command;

    /* 读取编码器增量和 IMU 数据，并立刻更新里程计。 */
    Encoder_Update();
    encoder = Encoder_GetState();
    imu_ready = ICM42688_ReadData(&imu);

    Odometry_Update(&g_carOdometry, encoder.left_delta, encoder.right_delta);

    /* 角度环开启且 IMU 正常时，计算左右轮差速修正量。 */
    if (command.angle_loop_enable && imu_ready) {
        angle_correction = AnglePID_Update(&g_angle_pid,
                                           command.target_angle_deg,
                                           imu.yaw_deg,
                                           imu.gyro_dps[2]);
    } else {
        PID_Reset(&g_angle_pid);
    }

    /* 角度修正量左减右加，使小车通过差速调整车头方向。 */
    left_target = (int16_t) (command.left_speed - angle_correction);
    right_target = (int16_t) (command.right_speed + angle_correction);

    if ((command.left_speed == 0) && (command.right_speed == 0) &&
        !command.angle_loop_enable) {
        reset_speed_loop();
        Motor_Stop();
        left_pwm = 0;
        right_pwm = 0;
    } else {
        /* 速度环输入是目标 tick 和本周期实测 tick，输出直接就是 PWM。 */
        left_pwm = SpeedPID_Update(&g_left_speed_pid, left_target,
                                   encoder.left_delta);
        right_pwm = SpeedPID_Update(&g_right_speed_pid, right_target,
                                    encoder.right_delta);
        Motor_SetPwm(left_pwm, right_pwm);
    }

    g_state.target_left_speed = command.left_speed;
    g_state.target_right_speed = command.right_speed;
    g_state.measured_left_speed = encoder.left_delta;
    g_state.measured_right_speed = encoder.right_delta;
    g_state.left_pwm = left_pwm;
    g_state.right_pwm = right_pwm;
    g_state.target_angle_deg = command.target_angle_deg;
    g_state.current_angle_deg = imu.yaw_deg;
    g_state.angle_correction = (float) angle_correction;
    g_state.angle_loop_enable = command.angle_loop_enable;
    g_state.imu_ready = imu_ready;
}

CarControlState_t Car_GetControlState(void)
{
    return g_state;
}
