#include "key.h"

#include "ti_msp_dl_config.h"

typedef enum {
    KEY_STATE_IDLE = 0,
    KEY_STATE_CONFIRM_PRESS,
    KEY_STATE_WAIT_RELEASE
} KeyDebounceState_t;

typedef enum {
    KEY_INDEX_MODE = 0,
    KEY_INDEX_START,
    KEY_INDEX_STOP,
    KEY_COUNT
} KeyIndex_t;

typedef struct {
    KeyDebounceState_t state;
    bool pin_high;
} KeyDebounce_t;

/*
 * 这三个就是按键标志位。
 *
 * key1_flag：第 1 个按键有效按下后变成 1。
 * key2_flag：第 2 个按键有效按下后变成 1。
 * key3_flag：第 3 个按键有效按下后变成 1。
 *
 * 使用方法：
 * if (key1_flag) {
 *     key1_flag = 0;
 *     // 这里写第 1 个按键对应的处理
 * }
 */
volatile uint8_t key1_flag;
volatile uint8_t key2_flag;
volatile uint8_t key3_flag;

static KeyDebounce_t g_keys[KEY_COUNT];

/*
 * 清空单个按键的消抖状态。
 * pin_high=true 表示默认松开，因为硬件使用内部上拉，松开时 GPIO 为高电平。
 */
static void key_clear(KeyDebounce_t *key)
{
    key->state = KEY_STATE_IDLE;
    key->pin_high = true;
}

/*
 * 读取三个按键的原始 GPIO 电平。
 * 返回值保持和原工程一致：true 表示松开，高电平；false 表示按下，低电平。
 */
static void key_read_gpio(bool level[KEY_COUNT])
{
    level[KEY_INDEX_MODE] =
        (DL_GPIO_readPins(GPIO_KEY_PORT, GPIO_KEY_PIN_KEY_MODE_PIN) != 0U);
    level[KEY_INDEX_START] =
        (DL_GPIO_readPins(GPIO_KEY_PORT, GPIO_KEY_PIN_KEY_START_PIN) != 0U);
    level[KEY_INDEX_STOP] =
        (DL_GPIO_readPins(GPIO_KEY_PORT, GPIO_KEY_PIN_KEY_STOP_PIN) != 0U);
}

/*
 * 单个按键的三态消抖状态机，写法参考原工程 key_()。
 * IDLE: 等待检测到低电平。
 * CONFIRM_PRESS: 再等一个 10ms 周期，确认不是抖动。
 * WAIT_RELEASE: 等待松手；松手时只产生一次按键事件。
 *
 * 返回 true 表示这个按键已经完成一次“按下 -> 松开”的有效动作。
 */
static bool key_update_one(KeyDebounce_t *key, bool pin_high)
{
    bool pressed_event = false;

    key->pin_high = pin_high;

    switch (key->state) {
        case KEY_STATE_IDLE:
            if (!pin_high) {
                key->state = KEY_STATE_CONFIRM_PRESS;
            }
            break;

        case KEY_STATE_CONFIRM_PRESS:
            if (!pin_high) {
                key->state = KEY_STATE_WAIT_RELEASE;
            } else {
                key->state = KEY_STATE_IDLE;
            }
            break;

        case KEY_STATE_WAIT_RELEASE:
            if (pin_high) {
                pressed_event = true;
                key->state = KEY_STATE_IDLE;
            }
            break;

        default:
            key_clear(key);
            break;
    }

    return pressed_event;
}

void CarKey_Init(void)
{
    uint8_t i;

    for (i = 0U; i < (uint8_t) KEY_COUNT; i++) {
        key_clear(&g_keys[i]);
    }

    key1_flag = 0U;
    key2_flag = 0U;
    key3_flag = 0U;
}

void CarKey_Update10ms(void)
{
    bool level[KEY_COUNT];

    key_read_gpio(level);

    if (key_update_one(&g_keys[KEY_INDEX_MODE], level[KEY_INDEX_MODE])) {
        key1_flag = 1U;
    }

    if (key_update_one(&g_keys[KEY_INDEX_START], level[KEY_INDEX_START])) {
        key2_flag = 1U;
    }

    if (key_update_one(&g_keys[KEY_INDEX_STOP], level[KEY_INDEX_STOP])) {
        key3_flag = 1U;
    }
}
