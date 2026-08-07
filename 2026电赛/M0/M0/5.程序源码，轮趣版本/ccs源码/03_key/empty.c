#include "ti_msp_dl_config.h"

int main(void)
{

        SYSCFG_DL_init();

        while (1)
        {
                //如果读取到的引脚值大于0，说明PA18引脚为高电平
	// If the pin value read is greater than 0, it means that the PA18 pin is high level.
                if( DL_GPIO_readPins(KEY_PORT, KEY_PIN_18_PIN) > 0 )
                {
                        DL_GPIO_setPins(LED1_PORT,LED1_PIN_2_PIN);  //LED控制输出高电平 LED control output high level
                }
                else//如果PA18引脚为低电平 If the PA18 pin is low
                {
                        DL_GPIO_clearPins(LED1_PORT,LED1_PIN_2_PIN);//LED控制输出低电平 LED control output low level
                }
        }
}