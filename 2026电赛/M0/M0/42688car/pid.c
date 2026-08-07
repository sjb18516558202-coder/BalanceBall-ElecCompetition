#include "pid.h"

#include "app_config.h"

float PID_WrapDegrees(float angle_deg)
{
    /* 让角度一直落在 -180 到 180 度之间，避免跨 0 度时误差过大。 */
    while (angle_deg > 180.0f) {
        angle_deg -= 360.0f;
    }
    while (angle_deg < -180.0f) {
        angle_deg += 360.0f;
    }
    return angle_deg;
}

void SpeedPID_Init(PIDController_t *pid)
{
    if (pid == 0) {
        return;
    }

    pid->kp = CAR_SPEED_KP;
    pid->ki = CAR_SPEED_KI;
    pid->kd = CAR_SPEED_KD;
    pid->output_max = CAR_PWM_MAX;
    PID_Reset(pid);
}

void AnglePID_Init(PIDController_t *pid)
{
    if (pid == 0) {
        return;
    }

    pid->kp = CAR_ANGLE_KP;
    pid->ki = CAR_ANGLE_KI;
    pid->kd = CAR_ANGLE_KD;
    pid->output_max = (int16_t) CAR_ANGLE_OUTPUT_LIMIT;
    PID_Reset(pid);
}

void PID_Reset(PIDController_t *pid)
{
    if (pid == 0) {
        return;
    }

    pid->last_error = 0.0f;
    pid->integral = 0.0f;
    pid->output = 0;
}

int16_t SpeedPID_Update(PIDController_t *pid, int16_t target_speed,
                        int32_t current_speed)
{
    float error;
    float delta_p;
    float delta_i;
    float delta_output;
    float output;

    if (pid == 0) {
        return 0;
    }

    /*
     * 速度环写法参考原模板工程：
     * error = 目标速度 - 当前速度
     * 输出增量 = Kp * (本次误差 - 上次误差) + Ki * 本次误差
     * 再把这次增量累加到原来的 PWM 输出上。
     */
    error = (float) target_speed - (float) current_speed;
    delta_p = pid->kp * (error - pid->last_error);
    delta_i = pid->ki * error;
    delta_output = delta_p + delta_i;
    output = (float) pid->output + delta_output;

    if (output > (float) pid->output_max) {
        output = (float) pid->output_max;
    }
    if (output < (float) (-pid->output_max)) {
        output = (float) (-pid->output_max);
    }

    pid->output = (int16_t) output;
    pid->last_error = error;
    return pid->output;
}

int16_t AnglePID_Update(PIDController_t *pid, float target_deg,
                        float current_deg, float gyro_z_dps)
{
    float error;
    float output;

    if (pid == 0) {
        return 0;
    }

    /*
     * 角度环写法也尽量按原模板工程来：
     * 1. 先算目标角和当前角的误差。
     * 2. 再把误差压到 -180 到 180 度。
     * 3. 比例项看当前偏差，积分项消除长期偏差，
     *    微分项直接使用陀螺仪 Z 轴角速度。
     */
    error = PID_WrapDegrees(target_deg - current_deg);
    pid->integral += error;

    if (pid->integral > CAR_ANGLE_INTEGRAL_LIMIT) {
        pid->integral = CAR_ANGLE_INTEGRAL_LIMIT;
    }
    if (pid->integral < -CAR_ANGLE_INTEGRAL_LIMIT) {
        pid->integral = -CAR_ANGLE_INTEGRAL_LIMIT;
    }

    output = pid->kp * error +
             pid->ki * pid->integral -
             pid->kd * gyro_z_dps;

    if (output > (float) pid->output_max) {
        output = (float) pid->output_max;
    }
    if (output < (float) (-pid->output_max)) {
        output = (float) (-pid->output_max);
    }

    pid->last_error = error;
    pid->output = (int16_t) output;
    return pid->output;
}
