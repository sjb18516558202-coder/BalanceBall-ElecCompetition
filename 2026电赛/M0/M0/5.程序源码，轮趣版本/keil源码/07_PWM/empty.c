#include "ti_msp_dl_config.h"

volatile unsigned int delay_times = 0;

//搭配滴答定时器的精准毫秒级延时 Precise millisecond delay with tick timer
void delay_ms(unsigned int ms)
{
    delay_times = ms;
    while( delay_times != 0 );
}

int main(void)
{
    int i = 0;

    SYSCFG_DL_init();

    while (1)
    {

        // 呼吸灯渐亮过程 Breathing light gradually brightens
        for (i = 0; i <= 999; i++)
        {
            // 设置 LED 亮度 Setting LED Brightness
            DL_TimerG_setCaptureCompareValue(PWM_LED_INST,i,GPIO_PWM_LED_C0_IDX);
            delay_ms(1);  // 延迟以控制亮度变化速度 Delay to control the speed of brightness change
        }
        // 呼吸灯渐暗过程 Breathing light dimming process
        for (i = 999; i > 0; i--)
        { 
            // 设置 LED 亮度 Setting LED Brightness
            DL_TimerG_setCaptureCompareValue(PWM_LED_INST,i,GPIO_PWM_LED_C0_IDX);
            delay_ms(1);  // 延迟以控制亮度变化速度 Delay to control the speed of brightness change
        }
    }
}

//滴答定时器的中断服务函数 Tick ??timer interrupt service function
void SysTick_Handler(void)
{
    if( delay_times != 0 )
    {
        delay_times--;
    }
}