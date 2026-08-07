#ifndef MOTOR_H_
#define MOTOR_H_

#include <stdint.h>

/* 初始化电机方向 GPIO 和 PWM 定时器，启动后默认保持停止。 */
void Motor_Init(void);

/*
 * 设置左右轮 PWM。
 * 正数代表正转，负数代表反转，0 代表停止；内部会自动限幅到 CAR_PWM_MAX。
 */
void Motor_SetPwm(int16_t left_pwm, int16_t right_pwm);

/* 关闭左右轮方向输出，并把 PWM 占空比恢复为 0。 */
void Motor_Stop(void);

#endif /* MOTOR_H_ */
