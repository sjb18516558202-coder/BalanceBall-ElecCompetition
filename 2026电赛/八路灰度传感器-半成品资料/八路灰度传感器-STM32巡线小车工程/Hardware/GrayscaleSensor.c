#include "stm32f10x.h"                  // 包含STM32F10x系列微控制器的头文件
#include "Motor.h"                     // 包含电机控制相关的头文件
#include "GrayscaleSensor.h"            // 包含灰度传感器相关的头文件

// 初始化灰度传感器的函数
void GrayscaleSensor_Init(void) {
    // 使能对应的GPIO时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    
    // 失能JTAG，释放对应引脚
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    // 定义一个GPIO结构体
    GPIO_InitTypeDef GPIO_InitStructure;

    // 初始化GPIOC的引脚C13和C14为上拉输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14; // 初始化C13和C14
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 设置GPIO速度为50MHz
    GPIO_Init(GPIOC, &GPIO_InitStructure);  // 应用配置

    // 初始化GPIOB的引脚B3、B4、B5、B6和B7为上拉输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7|GPIO_Pin_8; // 初始化B3到B8
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 设置GPIO速度为50MHz
    GPIO_Init(GPIOB, &GPIO_InitStructure);  // 应用配置
}

// 控制电机的函数
void motor(int Z_Lun, int Y_Lun) {
    // 设置左轮和右轮的速度
    LeftWheelBehind_Speed(Z_Lun);   // 设置左后轮速度
    LeftWheelFront_Speed(Z_Lun);    // 设置左前轮速度
    RightWheelBehind_Speed(Y_Lun);  // 设置右后轮速度
    RightWheelFront_Speed(Y_Lun);   // 设置右前轮速度
}

