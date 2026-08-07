#ifndef ENCODER_H_
#define ENCODER_H_

#include <stdint.h>

/*
 * EncoderState_t 保存编码器的本周期增量、累计值和原始计数器值。
 * 左轮使用 TIMG8 QEI 模式，右轮使用 TIMG6 硬件计数 A 相 + GPIO 采样 B 相推方向。
 */
typedef struct {
    /* 最近一个 10ms 控制周期内左右轮增量 tick。 */
    int32_t left_delta;
    int32_t right_delta;

    /* 上电或复位后的累计 tick，便于调试轮子转动方向和里程。 */
    int64_t left_total;
    int64_t right_total;

    /* 定时器当前原始计数值，主要用于排查硬件计数是否工作。 */
    uint32_t left_raw;
    uint32_t right_raw;
} EncoderState_t;

/* 启动编码器计数定时器并清零软件状态。 */
void Encoder_Init(void);

/* 清零软件累计值，并重新记录当前定时器计数作为基准。 */
void Encoder_Reset(void);

/* 读取硬件计数并计算本周期增量，通常每 10ms 调用一次。 */
void Encoder_Update(void);

/* 获取编码器状态快照。 */
EncoderState_t Encoder_GetState(void);

#endif /* ENCODER_H_ */
