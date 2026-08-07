#include "ti_msp_dl_config.h"
#include "stdio.h"

volatile uint16_t ADC_VALUE[20];//ADC采集的数据保存地址 The data collected by ADC is saved in the address

unsigned int adc_getValue(unsigned int number); //读取ADC的数据 Read ADC data
void uart0_send_string(char* str); //串口发送字符串 Send string via serial port

int main(void)
{
    char output_buff[50] = {0};
    unsigned int adc_value = 0;
    float voltage_value = 0;

    SYSCFG_DL_init();

    //设置DMA搬运的起始地址 Set the starting address of DMA transfer
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t) &ADC0->ULLMEM.MEMRES[0]);
    //设置DMA搬运的目的地址 Set the destination address of DMA transfer
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t) &ADC_VALUE[0]);
    //开启DMA Enable DMA
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
    //开启ADC转换 Start ADC conversion
    DL_ADC12_startConversion(ADC_VOLTAGE_INST);

    uart0_send_string("adc+dma Demo start\r\n");
    while (1)
    {
        //获取ADC数据 Get ADC data
        adc_value = adc_getValue(10);
        sprintf(output_buff, "adc value:%d\r\n", adc_value);
        uart0_send_string(output_buff);

        //将ADC采集的数据换算为电压 Convert the data collected by ADC into voltage
        voltage_value = adc_value/4095.0*3.3;
        sprintf(output_buff, "voltage value:%.2f\r\n", voltage_value);
        uart0_send_string(output_buff);

        delay_cycles(32000000);
    }
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

//串口发送字符串 Send string via serial port
void uart0_send_string(char* str)
{
    //当前字符串地址不在结尾 并且 字符串首地址不为空
    //The current string address is not at the end and the string first address is not empty
    while(*str!=0&&str!=0)
    {
         //当串口0忙的时候等待，不忙的时候再发送传进来的字符
         // Wait when serial port 0 is busy, and send the incoming characters when it is not busy
        while( DL_UART_isBusy(UART_0_INST) == true );
        //发送字符串首地址中的字符，并且在发送完成之后首地址自增
        // Send the characters in the first address of the string, and increment the first address after sending.
        DL_UART_Main_transmitData(UART_0_INST, *str++);
    }
}