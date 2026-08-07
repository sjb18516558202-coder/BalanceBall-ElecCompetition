#include "board.h"
#include <stdio.h>
#include "bsp_mpu6050.h"
#include "inv_mpu.h"

char buf[100];  // 定义缓冲区 Defining buffers

int main(void)
{
    // 开发板初始化 Development board initialization
    board_init();
    
    // 初始化 MPU6050 Initialize MPU6050
    MPU6050_Init();
    
    uint8_t ret = 1;
    float pitch = 0, roll = 0, yaw = 0;  // 欧拉角 Euler Angles

    printf("start\r\n");

    // DMP 初始化 DMP initialization
    while (mpu_dmp_init())
    {
        printf("DMP error\r\n");
        delay_ms(200);
    }
    
    printf("Initialization Data Succeed \r\n");

    while (1)
    {
        // 获取欧拉角 Get Euler angles
        if (mpu_dmp_get_data(&pitch, &roll, &yaw) == 0)
        {
            // 格式化数据并发送 Format data and send
            sprintf(buf, "pitch = %d, roll = %d, yaw = %d\n", (int)pitch, (int)roll, (int)yaw);
            uart0_send_string(buf);  // 串口发送 Serial port sending
        }
        else
        {
            // 获取数据失败 Failed to obtain data
            printf("Data get error\r\n");
        }

        delay_ms(200);  // 延时，根据实际采样率调整 Delay, adjusted according to the actual sampling rate
    }

}