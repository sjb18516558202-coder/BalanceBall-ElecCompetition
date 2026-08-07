#ifndef _CLOCK_H_
#define _CLOCK_H_

/*
 * clock.h 提供一个轻量级毫秒时间基准。
 * OLED 初始化、ICM42688 初始化以及阻塞等待都依赖这个 tick_ms。
 */
extern volatile unsigned long tick_ms;

/* 阻塞延时 num_ms 毫秒，使用前必须先调用 SysTick_Init() 并打开全局中断。 */
int mspm0_delay_ms(unsigned long num_ms);

/* 读取当前毫秒计数，成功返回 0，count 为空返回 1。 */
int mspm0_get_clock_ms(unsigned long *count);

/* 配置 SysTick 为 1ms 周期。 */
void SysTick_Init(void);

#endif  /* #ifndef _CLOCK_H_ */
