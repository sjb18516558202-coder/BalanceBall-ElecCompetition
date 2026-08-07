#ifndef KEY_H_
#define KEY_H_

#include <stdbool.h>
#include <stdint.h>

/*
 * 三个按键的标志位。
 *
 * 接法和原工程一致：按键一端接 GPIO，一端接 GND，GPIO 使用内部上拉。
 * 松开时 GPIO 读到 1，按下时 GPIO 读到 0。
 *
 * 软件逻辑也按原工程写法：按键“按下并松开”后，对应 flag 置 1。
 * 你在主循环里判断 flag == 1，然后写自己的处理代码，处理完手动清 0。
 *
 * 当前引脚：
 * key1_flag -> PB21，SysConfig 名称 GPIO_KEY_PIN_KEY_MODE_PIN。
 * key2_flag -> PB15，SysConfig 名称 GPIO_KEY_PIN_KEY_START_PIN。
 * key3_flag -> PB19，SysConfig 名称 GPIO_KEY_PIN_KEY_STOP_PIN。
 */
extern volatile uint8_t key1_flag;
extern volatile uint8_t key2_flag;
extern volatile uint8_t key3_flag;

/* 清空按键消抖状态和 key1_flag/key2_flag/key3_flag。 */
void CarKey_Init(void);

/*
 * 10ms 按键消抖扫描函数。
 * 这个函数只读 GPIO，并在确认按键有效后置位 flag。
 * 它不会控制电机，也不会自动清除 flag。
 */
void CarKey_Update10ms(void);

#endif /* KEY_H_ */
