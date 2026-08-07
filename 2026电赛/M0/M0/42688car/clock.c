#include "ti_msp_dl_config.h"
#include "clock.h"

/* 全局毫秒计数，由 SysTick_Handler() 每 1ms 自增一次。 */
volatile unsigned long tick_ms;

/* mspm0_delay_ms() 使用的起始时间缓存，单线程阻塞延时场景下使用即可。 */
volatile uint32_t start_time;

int mspm0_delay_ms(unsigned long num_ms)
{
    /*
     * 这里是简单阻塞延时。
     * “阻塞”的意思是：函数没有返回之前，CPU 会一直卡在这里等时间到。
     *
     * 注意：SysTick 中断必须已经打开，否则 tick_ms 不会增长，函数会一直等待。
     */
    start_time = tick_ms;
    while (tick_ms - start_time < num_ms);
    return 0;
}

int mspm0_get_clock_ms(unsigned long *count)
{
    /* count 为空说明调用者没有给存放地址，返回 1 表示参数错误。 */
    if (!count)
        return 1;

    /* 返回当前系统运行毫秒数，常用于传感器驱动里的超时或时间戳。 */
    count[0] = tick_ms;
    return 0;
}

void SysTick_Init(void)
{
    /*
     * SysTick 是 ARM 内核自带的系统定时器，不是普通外设定时器。
     * 这里把它配置成每 1ms 进一次中断。
     */
    DL_SYSTICK_config(CPUCLK_FREQ/1000);

    /*
     * NVIC_SetPriority() 用来设置中断优先级。
     * 数字越小优先级越高。
     * 这里给 SysTick 设成 0，表示优先级比较高，保证 1ms 计时尽量稳定。
     */
    NVIC_SetPriority(SysTick_IRQn, 0);
}

void SysTick_Handler(void)
{
    /*
     * SysTick_Handler() 是 SysTick 的中断服务函数。
     * 每进来一次，就说明又过去了 1ms。
     */
    tick_ms++;
}


