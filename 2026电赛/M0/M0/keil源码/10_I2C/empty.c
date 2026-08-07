#include "board.h"
#include <stdio.h>
#include "bsp_mpu6050.h"
#include "inv_mpu.h"

int main(void)
{
    //开发板初始化 Development board initialization
    board_init();
    
    MPU6050_Init();
        
    uint8_t ret = 1;
          
    float pitch=0,roll=0,yaw=0;   //欧拉角 Euler Angles

    printf("start\r\n");

    //DMP初始化 DMP Initialization
    while( mpu_dmp_init() )
    {
        printf("dmp error\r\n");
        delay_ms(200);
    }
	
    printf("Initialization Data Succeed \r\n");
	
    while(1) 
    {
        //获取欧拉角 Get Euler angles
        if( mpu_dmp_get_data(&pitch,&roll,&yaw) == 0 )
        { 
            printf("\r\npitch =%d\r\n", (int)pitch);
            printf("\r\nroll =%d\r\n", (int)roll);
            printf("\r\nyaw =%d\r\n", (int)yaw);
        }      
        delay_ms(200);//根据设置的采样率，不可设置延时过大 According to the set sampling rate, the delay cannot be set too large
    }

}