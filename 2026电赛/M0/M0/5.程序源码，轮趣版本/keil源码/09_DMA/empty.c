#include "ti_msp_dl_config.h"
#include "stdio.h"

volatile unsigned int delay_times = 0;
volatile bool gCheckADC;        //ADC采集成功标志位 ADC acquisition success flag
volatile uint16_t ADC_VALUE[20];

void delay_ms(unsigned int ms); //搭配滴答定时器的精准毫秒级延时 Precise millisecond delay with tick timer
unsigned int adc_getValue(unsigned int number);//读取ADC的数据 Read ADC data


/******************************串口重定向 Serial port redirection***************************************/
#if !defined(__MICROLIB)
//不使用微库的话就需要添加下面的函数
// If you don't use the micro library, you need to add the following function
#if (__ARMCLIB_VERSION <= 6000000)
//如果编译器是AC5  就定义下面这个结构体
//If the compiler is AC5, define the following structure
struct __FILE
{
        int handle;
};
#endif

FILE __stdout;

//定义_sys_exit()以避免使用半主机模式
// Define _sys_exit() to avoid using semihosting mode
void _sys_exit(int x)
{
        x = x;
}
#endif
//printf函数重定义 printf function redefinition
int fputc(int ch, FILE *stream)
{
        //当串口0忙的时候等待，不忙的时候再发送传进来的字符
				// Wait when serial port 0 is busy, and send the incoming characters when it is not busy
        while( DL_UART_isBusy(UART_0_INST) == true );

        DL_UART_Main_transmitData(UART_0_INST, ch);

        return ch;
}
/*********************************************************************/

int main(void)
{
        unsigned int adc_value = 0;
        unsigned int voltage_value = 0;
        int i = 0;

        SYSCFG_DL_init();

        //清除串口中断标志 Clear the serial port interrupt flag
        NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
        //开启串口中断 Enable serial port interrupt
        NVIC_EnableIRQ(UART_0_INST_INT_IRQN);

        //设置DMA搬运的起始地址 Set the starting address of DMA transfer
        DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t) &ADC0->ULLMEM.MEMRES[0]);
        //设置DMA搬运的目的地址 Set the destination address of DMA transfer
        DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t) &ADC_VALUE[0]);
        //开启DMA Enable DMA
        DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
        //开启ADC转换 Start ADC conversion
        DL_ADC12_startConversion(ADC_VOLTAGE_INST);

        printf("adc_dma Demo start\r\n");

        while (1)
        {
                //获取ADC数据 Get ADC data
                adc_value = adc_getValue(10);
                printf("adc value:%d\r\n", adc_value);

                //将ADC采集的数据换算为电压 Convert the data collected by ADC into voltage
                voltage_value = (int)((adc_value/4095.0*3.3)*100);

                printf("voltage value:%d.%d%d\r\n",
                voltage_value/100,
                voltage_value/10%10,
                voltage_value%10 );

                delay_ms(1000);
        }
}
//延时函数 Delay function
void delay_ms(unsigned int ms)
{
        delay_times = ms;
        while( delay_times != 0 );
}

//读取ADC的数据 Read ADC data
unsigned int adc_getValue(unsigned int number)
{
        unsigned int gAdcResult = 0;
        unsigned char i = 0;

        //采集多次累加 Collect multiple times and accumulate
        for( i = 0; i < number; i++ )
        {
                gAdcResult += ADC_VALUE[i];
        }
        //均值滤波 Mean Filter
        gAdcResult /= number;

        return gAdcResult;
}

//滴答定时器的中断服务函数 Tick ??timer interrupt service function
void SysTick_Handler(void)
{
        if( delay_times != 0 )
        {
                delay_times--;
        }
}