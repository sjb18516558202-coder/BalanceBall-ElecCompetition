#include "ti_msp_dl_config.h"
#include "stdio.h"

volatile bool gCheckADC;        //ADC采集成功标志位 ADC acquisition success flag
unsigned int adc_getValue(void);//读取ADC的数据 Read ADC data

//串口发送字符串 Send string via serial port
void uart0_send_string(char* str)
{
    //当前字符串地址不在结尾 并且 字符串首地址不为空 
    // The current string address is not at the end and the string's first address is not empty
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

int main(void)
{
    char output_buff[50] = {0};
    unsigned int adc_value = 0;
    float voltage_value = 0;

    SYSCFG_DL_init();

    //开启ADC中断 Enable ADC interrupt
    NVIC_EnableIRQ(ADC_VOLTAGE_INST_INT_IRQN);

    uart0_send_string("adc Demo start\r\n");
    while (1)
    {
        //获取ADC数据 Get ADC data
        adc_value = adc_getValue();
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
unsigned int adc_getValue(void)
{
        unsigned int gAdcResult = 0;

        //软件触发ADC开始转换 Software triggers ADC to start conversion
        DL_ADC12_startConversion(ADC_VOLTAGE_INST);
        //如果当前状态为正在转换中则等待转换结束 If the current state is in transition, wait for the transition to end.
        while (false == gCheckADC) {
            __WFE();
        }
        //获取数据 Get data
        gAdcResult = DL_ADC12_getMemResult(ADC_VOLTAGE_INST, ADC_VOLTAGE_ADCMEM_ADC_CH0);

        //清除标志位 Clear flag
        gCheckADC = false;

        return gAdcResult;
}

//ADC中断服务函数 ADC interrupt service function
void ADC_VOLTAGE_INST_IRQHandler(void)
{
    //查询并清除ADC中断 Query and clear ADC interrupt
    switch (DL_ADC12_getPendingInterrupt(ADC_VOLTAGE_INST))
    {
        //检查是否完成数据采集 Check whether data collection is completed
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            gCheckADC = true;//将标志位置1 Set the flag position to 1
            break;
        default:
            break;
    }
}