#include "motor.h"

#include "app_config.h"
#include "ti_msp_dl_config.h"

static int16_t clamp_pwm(int16_t pwm)
{
    /* PWM 限幅，防止 PID 输出超过定时器周期。 */
    if (pwm > CAR_PWM_MAX) {
        return CAR_PWM_MAX;
    }
    if (pwm < -CAR_PWM_MAX) {
        return -CAR_PWM_MAX;
    }
    return pwm;
}

static void set_pwm_compare(uint32_t channel, int16_t pwm_abs)
{
    uint32_t compare_value;

    if (pwm_abs < CAR_PWM_DEAD_BAND) {
        pwm_abs = 0;
    }

    /*
     * 当前 PWM 输出沿用原工程逻辑：比较值 = 1600 - pwm。
     * pwm 越大，占空比越大；pwm=0 时比较值为满周期。
     */
    compare_value = (uint32_t) (CAR_PWM_MAX - pwm_abs);
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, compare_value, channel);
}

static void set_left_direction(int16_t pwm)
{
    /* 左电机方向：正数前进，负数后退，0 时两个方向脚都拉低。 */
    if (pwm > 0) {
        DL_GPIO_setPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_PIN_LEFT_IN1_PIN);
        DL_GPIO_clearPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_PIN_LEFT_IN2_PIN);
    } else if (pwm < 0) {
        DL_GPIO_clearPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_PIN_LEFT_IN1_PIN);
        DL_GPIO_setPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_PIN_LEFT_IN2_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_MOTOR_DIR_PORT,
            GPIO_MOTOR_DIR_PIN_LEFT_IN1_PIN | GPIO_MOTOR_DIR_PIN_LEFT_IN2_PIN);
    }
}

static void set_right_direction(int16_t pwm)
{
    /* 右电机方向：如果实车方向相反，优先调电机线或在控制层取反。 */
    if (pwm > 0) {
        DL_GPIO_setPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_PIN_RIGHT_IN1_PIN);
        DL_GPIO_clearPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_PIN_RIGHT_IN2_PIN);
    } else if (pwm < 0) {
        DL_GPIO_clearPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_PIN_RIGHT_IN1_PIN);
        DL_GPIO_setPins(GPIO_MOTOR_DIR_PORT, GPIO_MOTOR_DIR_PIN_RIGHT_IN2_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_MOTOR_DIR_PORT,
            GPIO_MOTOR_DIR_PIN_RIGHT_IN1_PIN | GPIO_MOTOR_DIR_PIN_RIGHT_IN2_PIN);
    }
}

void Motor_Init(void)
{
    /* 初始化时先停电机，再启动 PWM 计数器，避免上电瞬间误动作。 */
    Motor_Stop();
    DL_TimerA_startCounter(PWM_MOTOR_INST);
}

void Motor_SetPwm(int16_t left_pwm, int16_t right_pwm)
{
    /* 统一的电机输出函数：控制层只需要给左右 PWM 正负值。 */
    int16_t left_abs;
    int16_t right_abs;

    left_pwm = clamp_pwm(left_pwm);
    right_pwm = clamp_pwm(right_pwm);

    set_left_direction(left_pwm);
    set_right_direction(right_pwm);

    left_abs = (left_pwm < 0) ? -left_pwm : left_pwm;
    right_abs = (right_pwm < 0) ? -right_pwm : right_pwm;

    set_pwm_compare(DL_TIMER_CC_0_INDEX, left_abs);
    set_pwm_compare(DL_TIMER_CC_1_INDEX, right_abs);
}

void Motor_Stop(void)
{
    DL_GPIO_clearPins(GPIO_MOTOR_DIR_PORT,
        GPIO_MOTOR_DIR_PIN_LEFT_IN1_PIN | GPIO_MOTOR_DIR_PIN_LEFT_IN2_PIN |
        GPIO_MOTOR_DIR_PIN_RIGHT_IN1_PIN | GPIO_MOTOR_DIR_PIN_RIGHT_IN2_PIN);
    set_pwm_compare(DL_TIMER_CC_0_INDEX, 0);
    set_pwm_compare(DL_TIMER_CC_1_INDEX, 0);
}
