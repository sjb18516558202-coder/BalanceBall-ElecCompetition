/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include "stdio.h"
#include "motor_read_enc.h"
#include "motor_set_speed.h"


volatile unsigned char uart_data = 0;
volatile unsigned int delay_times = 0;
void delay_ms(unsigned int ms);


extern volatile int16_t	modbus_date[8];					//存储累计编码器值
extern volatile uint8_t modbus_rx_frame_done;   // 编码器读取完成标志位






int16_t v1=0;//范围 +-4000
int16_t v2=0;
int16_t v3=0;
int16_t v4=0;

int main(void)
{
    SYSCFG_DL_init();
		NVIC_ClearPendingIRQ(MSPMotor_INST_INT_IRQN);//串口
	  NVIC_EnableIRQ(MSPMotor_INST_INT_IRQN);
		
		NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);//1ms
		NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);	
		DL_TimerA_startCounter(TIMER_0_INST);
		
	
		delay_ms(50);//50ms延时
	
	

    while (1) {
				set_pwm_duty(v1, v2, v3, v4);

			
				delay_ms(1);//5ms延时
			
			


    }
}


void MSPMotor_INST_IRQHandler(void)//串口中断函数---获取4个电机的编码器值和速度
{
    switch( DL_UART_getPendingInterrupt(MSPMotor_INST) )
    {
        case DL_UART_IIDX_RX:


                uart_data = DL_UART_Main_receiveData(MSPMotor_INST);

                Modbus_ParseFrame(uart_data);


            break;

        default:
            break;
    }
}









uint8_t T0_Count=0;
void TIMER_0_INST_IRQHandler(void)//1ms
{ 	
		switch( DL_TimerG_getPendingInterrupt(TIMER_0_INST) )
    {     
        case DL_TIMER_IIDX_ZERO:
						
				
				
						break;
				default:

        break;
		}
	
	
		
}





void delay_ms(unsigned int ms)
{
    delay_times = ms;
    while( delay_times != 0 );
}

//滴答定时器中断服务函数
void SysTick_Handler(void)
{
    if( delay_times != 0 )
    {
        delay_times--;
    }
}

