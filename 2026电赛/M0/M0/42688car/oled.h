#ifndef OLED_H_
#define OLED_H_

#include <stdint.h>

#include "ti_msp_dl_config.h"

/*
 * OLED 模块说明：
 * 1. 本模块驱动常见 SSD1306 I2C OLED，地址为 0x78。
 * 2. 本工程使用 GPIO 模拟 I2C，不占用硬件 I2C 外设。
 * 3. 当前 OLED_SCL = PB12，OLED_SDA = PB13。
 */

#define OLED_CMD  (0U)   /* 写命令 */
#define OLED_DATA (1U)   /* 写显示数据 */

#define OLED_SCL_SET() \
    (DL_GPIO_setPins(GPIO_OLED_PORT, GPIO_OLED_PIN_OLED_SCL_PIN))
#define OLED_SCL_CLR() \
    (DL_GPIO_clearPins(GPIO_OLED_PORT, GPIO_OLED_PIN_OLED_SCL_PIN))
#define OLED_SDA_SET() \
    (DL_GPIO_setPins(GPIO_OLED_PORT, GPIO_OLED_PIN_OLED_SDA_PIN))
#define OLED_SDA_CLR() \
    (DL_GPIO_clearPins(GPIO_OLED_PORT, GPIO_OLED_PIN_OLED_SDA_PIN))

void OLED_Init(void);
void OLED_Clear(void);
void OLED_DisplayOn(void);
void OLED_DisplayOff(void);
void OLED_ColorTurn(uint8_t enable);
void OLED_DisplayTurn(uint8_t enable);
void OLED_SetPos(uint8_t x, uint8_t y);
void OLED_WriteByte(uint8_t dat, uint8_t mode);
void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size_y);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size_y);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len,
                  uint8_t size_y);
void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t num, uint8_t len,
                        uint8_t size_y);

#endif /* OLED_H_ */
