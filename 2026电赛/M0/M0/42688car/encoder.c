#include "encoder.h"

#include <stdbool.h>

#include "app_config.h"
#include "ti_msp_dl_config.h"

static EncoderState_t g_encoder;
static uint32_t g_last_left_count;
static uint32_t g_last_right_count;

static int32_t timer_delta_u16(uint32_t now, uint32_t last)
{
    /*
     * 定时器是 16 位计数，使用 int16_t 差值可以自然处理 65535 回绕。
     * 只要 10ms 内计数变化不超过 32767，就不会算错方向。
     */
    return (int32_t) ((int16_t) ((now - last) & 0xFFFFU));
}

static int32_t apply_direction_invert(int32_t value, int invert)
{
    return (invert != 0) ? -value : value;
}

void Encoder_Init(void)
{
    /* 左轮 QEI 和右轮边沿计数都由定时器硬件统计，不再使用外部 GPIO 中断。 */
    DL_TimerG_startCounter(QEI_LEFT_INST);
    DL_TimerG_startCounter(ENCODER_RIGHT_COUNT_INST);
    Encoder_Reset();
}

void Encoder_Reset(void)
{
    g_last_left_count = DL_TimerG_getTimerCount(QEI_LEFT_INST) & 0xFFFFU;
    g_last_right_count =
        DL_TimerG_getTimerCount(ENCODER_RIGHT_COUNT_INST) & 0xFFFFU;

    g_encoder.left_delta = 0;
    g_encoder.right_delta = 0;
    g_encoder.left_total = 0;
    g_encoder.right_total = 0;
    g_encoder.left_raw = g_last_left_count;
    g_encoder.right_raw = g_last_right_count;
}

void Encoder_Update(void)
{
    uint32_t now_left;
    uint32_t now_right;
    int32_t left_delta;
    int32_t right_delta;
    int32_t right_raw_delta;
    bool right_b_high;

    /* 读取当前硬件计数值。 */
    now_left = DL_TimerG_getTimerCount(QEI_LEFT_INST) & 0xFFFFU;
    now_right = DL_TimerG_getTimerCount(ENCODER_RIGHT_COUNT_INST) & 0xFFFFU;

    left_delta = timer_delta_u16(now_left, g_last_left_count);
    right_raw_delta = timer_delta_u16(now_right, g_last_right_count);

#if CAR_RIGHT_EDGE_TIMER_COUNTS_DOWN
    right_raw_delta = -right_raw_delta;
#endif

    /*
     * 右轮只有 A 相进入 TIMG6 边沿计数，B 相用 GPIO 采样判断方向。
     * 如果方向不对，修改 app_config.h 中的 CAR_RIGHT_ENCODER_DIR_INVERT。
     */
    right_b_high = (DL_GPIO_readPins(GPIO_ENCODER_RIGHT_PORT,
                        GPIO_ENCODER_RIGHT_PIN_RIGHT_ENCODER_B_PIN) != 0U);
    right_delta = right_b_high ? right_raw_delta : -right_raw_delta;
    right_delta *= CAR_RIGHT_ENCODER_EDGE_SCALE;

    left_delta = apply_direction_invert(left_delta, CAR_LEFT_ENCODER_DIR_INVERT);
    right_delta =
        apply_direction_invert(right_delta, CAR_RIGHT_ENCODER_DIR_INVERT);

    g_last_left_count = now_left;
    g_last_right_count = now_right;

    g_encoder.left_delta = left_delta;
    g_encoder.right_delta = right_delta;
    g_encoder.left_total += left_delta;
    g_encoder.right_total += right_delta;
    g_encoder.left_raw = now_left;
    g_encoder.right_raw = now_right;
}

EncoderState_t Encoder_GetState(void)
{
    return g_encoder;
}
