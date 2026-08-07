#ifndef PID_H_
#define PID_H_

#include <stdint.h>

typedef struct {
    float kp;          /* 比例系数 */
    float ki;          /* 积分系数 */
    float kd;          /* 微分系数 */
    float last_error;  /* 上一次误差 */
    float integral;    /* 积分累计值 */
    int16_t output;    /* 当前输出，速度环里就是当前 PWM */
    int16_t output_max;/* 输出最大值 */
} PIDController_t;

/* 速度环初始化，参数直接使用 app_config.h 里的原工程赋值。 */
void SpeedPID_Init(PIDController_t *pid);

/* 角度环初始化，参数直接使用 app_config.h 里的原工程赋值。 */
void AnglePID_Init(PIDController_t *pid);

/* 清零误差、积分和输出。 */
void PID_Reset(PIDController_t *pid);

/* 速度环更新函数，写法尽量按原模板工程的增量式 PI。 */
int16_t SpeedPID_Update(PIDController_t *pid, int16_t target_speed,
                        int32_t current_speed);

/* 角度环更新函数，输出左右轮差速修正量。 */
int16_t AnglePID_Update(PIDController_t *pid, float target_deg,
                        float current_deg, float gyro_z_dps);

/* 把角度限制到 -180 到 180 度。 */
float PID_WrapDegrees(float angle_deg);

#endif /* PID_H_ */
