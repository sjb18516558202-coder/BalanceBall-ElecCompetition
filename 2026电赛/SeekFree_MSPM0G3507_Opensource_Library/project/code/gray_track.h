#ifndef GRAY_TRACK_H
#define GRAY_TRACK_H

#include "zf_common_headfile.h"

//================================================= 八路数字灰度循迹配置 =================================================
// 说明：你的灰度传感器是 OUT1~OUT8 独立数字输出版本，不使用 GS08RA 的 S0/S1/S2/ADC 方案。
// 使用前请把下面 8 个 GPIO_MAX 改成实际连接的 GPIO，例如 A0、A1、B3 等。
// 保持 GPIO_MAX 时表示“引脚未配置”，程序会安全停车，避免误动作。
#define GRAY_TRACK_OUT1_PIN             ( B26 )      // 最左侧第 1 路灰度 OUT1（黄）
#define GRAY_TRACK_OUT2_PIN             ( B22 )      // 第 2 路灰度 OUT2（蓝）
#define GRAY_TRACK_OUT3_PIN             ( B21 )      // 第 3 路灰度 （绿）
#define GRAY_TRACK_OUT4_PIN             ( B23 )      // 第 4 路灰度 OUT4（白）
#define GRAY_TRACK_OUT5_PIN             ( B11 )      // 第 5 路灰度 OUT5（棕）
#define GRAY_TRACK_OUT6_PIN             ( A27 )      // 第 6 路灰度 OUT6（橙）
#define GRAY_TRACK_OUT7_PIN             ( B10 )      // 第 7 路灰度 OUT7（红）
#define GRAY_TRACK_OUT8_PIN             ( A26 )      // 最右侧第 8 路灰度 OUT8（黑）

// 黑线有效电平：如果实测黑线输出 1，保持 GPIO_HIGH；如果黑线输出 0，改成 GPIO_LOW。
#define GRAY_TRACK_BLACK_LEVEL          ( GPIO_HIGH )

// 循迹速度参数：先用小速度调试，确认电机方向和灰度方向正确后再逐步增大。
#define GRAY_TRACK_BASE_SPEED           ( 20 )            // 基础前进速度
#define GRAY_TRACK_KP                   ( 1.8 )            // 比例转向系数，越大转向越猛
#define GRAY_TRACK_KD                   ( 1 )            // 微分抑制系数，抑制误差变化过快导致的左右抖动
#define GRAY_TRACK_SPEED_LIMIT          ( 35 )           // 电机指令限幅，防止调试时速度过大

// 电机方向修正：如果某一侧电机方向反了，把对应符号改为 -1。
#define GRAY_TRACK_LEFT_SIGN            ( 1 )
#define GRAY_TRACK_RIGHT_SIGN           ( 1 )

//================================================= 八路数字灰度循迹配置 =================================================

extern uint8  gray_track_value[8];       // 二值化后的 8 路灰度值：1 表示检测到黑线，0 表示未检测到黑线
extern uint8  gray_track_raw[8];         // 8 路灰度 GPIO 原始电平：1 表示高电平，0 表示低电平
extern int16  gray_track_error;          // 当前循迹偏差：负数表示黑线偏左，正数表示黑线偏右
extern uint8  gray_track_lost_line;      // 丢线标志：1 表示 8 路都没有检测到黑线

void  gray_track_init(void);
void  gray_track_read(void);
void  gray_track_run(void);
uint8 gray_track_is_ready(void);

#endif
