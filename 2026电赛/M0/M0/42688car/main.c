#include "ti_msp_dl_config.h"

#include <stdbool.h>

#include "clock.h"
#include "control.h"
#include "display.h"
#include "icm42688.h"
#include "key.h"
#include "uart_comm.h"

/*
 * main.c 是整个工程的入口文件：
 * 1. 初始化 SysConfig 生成的底层外设。
 * 2. 初始化 SysTick、UART、OLED、ICM42688、按键和控制模块。
 * 3. 在 10 ms 定时器中断里执行闭环控制、按键消抖和 OLED 刷新调度。
 * 4. 在主循环里写你自己的按键逻辑，并低频刷新 OLED。
 */

static volatile bool g_display_update_pending = false;
static volatile uint8_t g_display_divider = 0U;

/*
 * OLED 不在中断里直接刷新。
 * 这里每 10 次 10ms 中断置一次标志，相当于 100ms 刷新一次屏幕。
 */
static void schedule_display_update_10ms(void)
{
    g_display_divider++;
    if (g_display_divider >= 10U) {
        g_display_divider = 0U;
        g_display_update_pending = true;
    }
}

/*
 * 10ms 周期任务入口：
 * 1. Car_ControlUpdate10ms() 做速度环、角度环和电机 PWM 更新。
 * 2. CarKey_Update10ms() 只做按键消抖，确认按下后把 key1/2/3_flag 置 1。
 * 3. schedule_display_update_10ms() 只置 OLED 刷新标志，不在中断里刷屏。
 */
static void run_10ms_tasks(void)
{
    Car_ControlUpdate10ms();
    CarKey_Update10ms();
    schedule_display_update_10ms();
}

int main(void)
{
    /* 按照 SysConfig 生成的配置，把 GPIO、定时器、串口、SPI 等外设初始化好。 */
    SYSCFG_DL_init();

    /* 把 Cortex-M0 的 SysTick 配成 1ms 中断一次，后面延时和计时都靠它。 */
    SysTick_Init();

    /* 打开全局中断。没有这句，定时器中断、串口中断、SysTick 中断都进不来。 */
    __enable_irq();

    /* 初始化三路串口的接收缓冲区，并打开对应的串口中断。 */
    CarUart_Init();

    /* 初始化 OLED，后面 Display_Update() 才能正常刷屏。 */
    Display_Init();

    /* 初始化 IMU，并把当前航向角当作 0 度起点。 */
    ICM42688_Init();
    ICM42688_SetYaw(0.0f);

    /* 初始化按键消抖状态和三个按键标志位。 */
    CarKey_Init();

    /* 初始化电机、编码器和 PID。 */
    Car_ControlInit();

    /* 先清掉一次可能残留的定时器中断申请，避免刚开中断就误进一次中断。 */
    NVIC_ClearPendingIRQ(CONTROL_TIMER_INST_INT_IRQN);

    /* 只打开 CONTROL_TIMER 这个中断源，对应 10ms 进入一次控制中断。 */
    NVIC_EnableIRQ(CONTROL_TIMER_INST_INT_IRQN);

    /* 再次打开全局中断，写两次也没关系，目的是保证中断一定处于开启状态。 */
    __enable_irq();

    /* 上电先让电机目标为 0，保证小车默认静止。 */
    Car_MotorControl(0, 0, false, 0.0f);

    while (1) {
        /*
         * 按键标志位在 key.h/key.c 里：
         * key1_flag、key2_flag、key3_flag。
         * 某个按键按下并松手后，对应 flag 会变成 1。
         * 你可以在这里判断，处理完以后自己清 0。
         */

        if (g_display_update_pending) {
            g_display_update_pending = false;
            Display_Update();
        }

        /*
         * __WFI() = Wait For Interrupt
         * 意思是“CPU 先停下来等中断”。
         *
         * 没有中断时，CPU 不会在 while(1) 里一直空转；
         * 一旦 10ms 定时器中断、串口中断等来了，CPU 会先去执行中断函数，
         * 中断执行完，再回到这里继续往下跑。
         */
        __WFI();
    }
}

void CONTROL_TIMER_INST_IRQHandler(void)
{
    /*
     * IRQHandler 就是“中断服务函数”。
     * 当 TIMA1 这个 10ms 定时器产生中断时，CPU 会自动跳到这个函数里执行。
     *
     * CONTROL_TIMER 只打开了 ZERO_EVENT 中断。
     * 读取 pending interrupt 会取得本次中断编号，并清掉对应的中断标志。
     */
    switch (DL_TimerA_getPendingInterrupt(CONTROL_TIMER_INST)) {
        case DL_TIMERA_IIDX_ZERO:
            run_10ms_tasks();
            break;

        default:
            break;
    }
}
