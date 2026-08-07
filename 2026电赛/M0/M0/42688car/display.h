#ifndef DISPLAY_H_
#define DISPLAY_H_

/* 初始化 OLED，并显示启动提示。 */
void Display_Init(void);

/*
 * 刷新 OLED 状态页，建议在主循环中低频调用，不要放在中断里。
 * 当前显示内容包括左右轮速度、PWM、角度环状态、里程计、IMU 和三路 UART 缓冲字节数。
 */
void Display_Update(void);

#endif /* DISPLAY_H_ */
