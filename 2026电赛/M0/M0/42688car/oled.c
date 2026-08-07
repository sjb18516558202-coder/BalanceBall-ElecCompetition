#include "oled.h"

#include "clock.h"
#include "oledfont.h"

/*
 * OLED 底层驱动：
 * - 负责模拟 I2C 时序。
 * - 负责向 SSD1306 写命令和显存数据。
 * - 负责提供字符、字符串、数字显示函数。
 */

static void oled_delay_short(void)
{
    /* 留出 GPIO 电平稳定时间，线长时可适当增加 NOP 数量。 */
    __NOP();
    __NOP();
    __NOP();
}

static void oled_i2c_start(void)
{
    /* 起始条件：SCL 高电平时 SDA 从高变低。 */
    OLED_SDA_SET();
    OLED_SCL_SET();
    oled_delay_short();
    OLED_SDA_CLR();
    oled_delay_short();
    OLED_SCL_CLR();
}

static void oled_i2c_stop(void)
{
    /* 停止条件：SCL 高电平时 SDA 从低变高。 */
    OLED_SDA_CLR();
    OLED_SCL_SET();
    oled_delay_short();
    OLED_SDA_SET();
    oled_delay_short();
}

static void oled_i2c_wait_ack(void)
{
    /* 简化处理：不读取 ACK，只保持一拍时钟。 */
    OLED_SDA_SET();
    oled_delay_short();
    OLED_SCL_SET();
    oled_delay_short();
    OLED_SCL_CLR();
}

static void oled_send_byte(uint8_t dat)
{
    uint8_t i;

    for (i = 0U; i < 8U; i++) {
        OLED_SCL_CLR();
        if ((dat & 0x80U) != 0U) {
            OLED_SDA_SET();
        } else {
            OLED_SDA_CLR();
        }
        oled_delay_short();
        OLED_SCL_SET();
        oled_delay_short();
        OLED_SCL_CLR();
        dat <<= 1;
    }
}

static uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1U;

    while (n-- != 0U) {
        result *= m;
    }
    return result;
}

void OLED_WriteByte(uint8_t dat, uint8_t mode)
{
    /*
     * SSD1306 7 位地址常见为 0x3C，写操作地址字节为 0x78。
     * 控制字节 0x00 表示命令，0x40 表示显示数据。
     */
    oled_i2c_start();
    oled_send_byte(0x78U);
    oled_i2c_wait_ack();
    oled_send_byte((mode == OLED_DATA) ? 0x40U : 0x00U);
    oled_i2c_wait_ack();
    oled_send_byte(dat);
    oled_i2c_wait_ack();
    oled_i2c_stop();
}

void OLED_SetPos(uint8_t x, uint8_t y)
{
    /* y 为页地址，每页 8 个像素高；x 为列地址。 */
    OLED_WriteByte((uint8_t) (0xB0U + y), OLED_CMD);
    OLED_WriteByte((uint8_t) (((x & 0xF0U) >> 4U) | 0x10U), OLED_CMD);
    OLED_WriteByte((uint8_t) (x & 0x0FU), OLED_CMD);
}

void OLED_DisplayOn(void)
{
    OLED_WriteByte(0x8DU, OLED_CMD);
    OLED_WriteByte(0x14U, OLED_CMD);
    OLED_WriteByte(0xAFU, OLED_CMD);
}

void OLED_DisplayOff(void)
{
    OLED_WriteByte(0x8DU, OLED_CMD);
    OLED_WriteByte(0x10U, OLED_CMD);
    OLED_WriteByte(0xAEU, OLED_CMD);
}

void OLED_ColorTurn(uint8_t enable)
{
    OLED_WriteByte((enable == 0U) ? 0xA6U : 0xA7U, OLED_CMD);
}

void OLED_DisplayTurn(uint8_t enable)
{
    if (enable == 0U) {
        OLED_WriteByte(0xC8U, OLED_CMD);
        OLED_WriteByte(0xA1U, OLED_CMD);
    } else {
        OLED_WriteByte(0xC0U, OLED_CMD);
        OLED_WriteByte(0xA0U, OLED_CMD);
    }
}

void OLED_Clear(void)
{
    uint8_t page;
    uint8_t col;

    for (page = 0U; page < 8U; page++) {
        OLED_WriteByte((uint8_t) (0xB0U + page), OLED_CMD);
        OLED_WriteByte(0x00U, OLED_CMD);
        OLED_WriteByte(0x10U, OLED_CMD);
        for (col = 0U; col < 128U; col++) {
            OLED_WriteByte(0x00U, OLED_DATA);
        }
    }
}

void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size_y)
{
    uint8_t c;
    uint8_t size_x;
    uint16_t i;
    uint16_t size_bytes;

    if ((chr < ' ') || (chr > '~')) {
        chr = ' ';
    }

    c = (uint8_t) (chr - ' ');
    size_x = (uint8_t) (size_y / 2U);
    size_bytes = (size_y == 8U) ? 6U :
        (uint16_t) ((size_y / 8U + ((size_y % 8U) ? 1U : 0U)) * size_x);

    OLED_SetPos(x, y);
    for (i = 0U; i < size_bytes; i++) {
        if ((size_y != 8U) && ((i % size_x) == 0U)) {
            OLED_SetPos(x, y++);
        }

        if (size_y == 8U) {
            OLED_WriteByte(asc2_0806[c][i], OLED_DATA);
        } else if (size_y == 16U) {
            OLED_WriteByte(asc2_1608[c][i], OLED_DATA);
        } else {
            return;
        }
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size_y)
{
    uint8_t step;

    if (str == 0) {
        return;
    }

    step = (size_y == 8U) ? 6U : (uint8_t) (size_y / 2U);
    while (*str != '\0') {
        OLED_ShowChar(x, y, *str, size_y);
        x = (uint8_t) (x + step);
        str++;
        if (x > (uint8_t) (128U - step)) {
            break;
        }
    }
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len,
                  uint8_t size_y)
{
    uint8_t i;
    uint8_t temp;
    uint8_t step = (size_y == 8U) ? 8U : (uint8_t) (size_y / 2U);

    for (i = 0U; i < len; i++) {
        temp = (uint8_t) ((num / oled_pow(10U, (uint8_t) (len - i - 1U))) % 10U);
        OLED_ShowChar((uint8_t) (x + step * i), y, (char) ('0' + temp), size_y);
    }
}

void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t num, uint8_t len,
                        uint8_t size_y)
{
    if (num < 0) {
        OLED_ShowChar(x, y, '-', size_y);
        OLED_ShowNum((uint8_t) (x + 8U), y, (uint32_t) (-num), len, size_y);
    } else {
        OLED_ShowChar(x, y, '+', size_y);
        OLED_ShowNum((uint8_t) (x + 8U), y, (uint32_t) num, len, size_y);
    }
}

void OLED_Init(void)
{
    /* OLED 上电后等待内部电源稳定。 */
    mspm0_delay_ms(200U);

    OLED_WriteByte(0xAEU, OLED_CMD);
    OLED_WriteByte(0x00U, OLED_CMD);
    OLED_WriteByte(0x10U, OLED_CMD);
    OLED_WriteByte(0x40U, OLED_CMD);
    OLED_WriteByte(0x81U, OLED_CMD);
    OLED_WriteByte(0xCFU, OLED_CMD);
    OLED_WriteByte(0xA1U, OLED_CMD);
    OLED_WriteByte(0xC8U, OLED_CMD);
    OLED_WriteByte(0xA6U, OLED_CMD);
    OLED_WriteByte(0xA8U, OLED_CMD);
    OLED_WriteByte(0x3FU, OLED_CMD);
    OLED_WriteByte(0xD3U, OLED_CMD);
    OLED_WriteByte(0x00U, OLED_CMD);
    OLED_WriteByte(0xD5U, OLED_CMD);
    OLED_WriteByte(0x80U, OLED_CMD);
    OLED_WriteByte(0xD9U, OLED_CMD);
    OLED_WriteByte(0xF1U, OLED_CMD);
    OLED_WriteByte(0xDAU, OLED_CMD);
    OLED_WriteByte(0x12U, OLED_CMD);
    OLED_WriteByte(0xDBU, OLED_CMD);
    OLED_WriteByte(0x40U, OLED_CMD);
    OLED_WriteByte(0x20U, OLED_CMD);
    OLED_WriteByte(0x02U, OLED_CMD);
    OLED_WriteByte(0x8DU, OLED_CMD);
    OLED_WriteByte(0x14U, OLED_CMD);
    OLED_WriteByte(0xA4U, OLED_CMD);
    OLED_WriteByte(0xA6U, OLED_CMD);
    OLED_Clear();
    OLED_WriteByte(0xAFU, OLED_CMD);
}
