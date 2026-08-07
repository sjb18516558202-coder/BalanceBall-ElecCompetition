#ifndef __TRACKING_H  // 检查是否已经定义了__TRACKING_H，防止头文件内容被重复包含
#define __TRACKING_H  // 定义__TRACKING_H，表示该头文件的内容现在被包含

#include "stm32f10x.h"                  // 包含STM32F10x系列微控制器的头文件
#include "Motor.h"                     // 包含电机控制相关的头文件
#include "GrayscaleSensor.h"            // 包含灰度传感器相关的头文件

// 函数声明
void Tracking_8Sensors(void);

#endif // __TRACKING_H

//这里提到的内外圈是专门特指2022年电赛C题的赛道
//C题赛道：https://blog.csdn.net/qq_52608074/article/details/126453231

