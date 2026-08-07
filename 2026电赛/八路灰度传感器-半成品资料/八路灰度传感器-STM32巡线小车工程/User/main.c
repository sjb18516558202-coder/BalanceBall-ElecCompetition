#include "stm32f10x.h"                  // 包含STM32F10x系列微控制器的头文件
#include "Delay.h"                      // 包含延时函数的头文件
#include "Motor.h"                      // 包含电机控制相关的头文件
#include "GrayscaleSensor.h"             // 包含灰度传感器相关的头文件
#include "Tracking.h"                   // 包含循迹相关的头文件

int main(void) {
    // 初始化电机
    Motor_Init();
    // 初始化灰度传感器
    GrayscaleSensor_Init();
    // 无限循环，持续执行循迹操作
    while (1) {
        // 调用循迹函数，执行循迹
        Tracking_8Sensors();
    }

}

