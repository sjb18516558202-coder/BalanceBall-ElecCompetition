#ifndef __GRAYSCALESENSOR_H
#define __GRAYSCALESENSOR_H

#include "stm32f10x.h"                  // STM32F10x系列微控制器的设备头文件
#include "Motor.h"                      // 电机控制相关的头文件

// 函数声明
void motor(int Z_Lun, int Y_Lun);       // 控制电机速度的函数
void GrayscaleSensor_Init(void);        // 初始化灰度传感器的函数
void Timer_Init(void);                 // 初始化定时器的函数

// 宏定义，用于读取灰度传感器的输出数据
#define D1         	GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_14)  // 读取GPIOC的第14位引脚的数据，连接到OUT1
#define D2         	GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13)  // 读取GPIOC的第13位引脚的数据，连接到OUT2
#define D3         	GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)   // 读取GPIOB的第7位引脚的数据，连接到OUT3
#define D4         	GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6)   // 读取GPIOB的第6位引脚的数据，连接到OUT4
#define D5         	GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5)   // 读取GPIOB的第5位引脚的数据，连接到OUT5
#define D6         	GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4)   // 读取GPIOB的第4位引脚的数据，连接到OUT6
#define D7         	GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3)   // 读取GPIOB的第3位引脚的数据，连接到OUT7
#define D8         	GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8)   // 读取GPIOB的第8位引脚的数据，连接到OUT8


#endif // __GRAYSCALESENSOR_H

