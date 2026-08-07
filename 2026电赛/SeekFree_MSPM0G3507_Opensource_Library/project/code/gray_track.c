#include "gray_track.h"
#include "motorsetspeed.h"

uint8 gray_track_value[8] = {0};
uint8 gray_track_raw[8] = {0};
int16 gray_track_error = 0;
uint8 gray_track_lost_line = 1;

static uint8 gray_track_ready = 0;
static int16 gray_track_last_error = 0;
static int16 gray_track_last_run_error = 0;

// 从左到右 8 路灰度的权重。黑线越靠左，误差越负；黑线越靠右，误差越正。
static const int16 gray_track_weight[8] = {-7, -5, -3, -1, 1, 3, 5, 7};

static const gpio_pin_enum gray_track_pin[8] =
{
    GRAY_TRACK_OUT1_PIN,
    GRAY_TRACK_OUT2_PIN,
    GRAY_TRACK_OUT3_PIN,
    GRAY_TRACK_OUT4_PIN,
    GRAY_TRACK_OUT5_PIN,
    GRAY_TRACK_OUT6_PIN,
    GRAY_TRACK_OUT7_PIN,
    GRAY_TRACK_OUT8_PIN,
};

static int16 gray_track_limit_speed(int16 speed)
{
    if(speed > GRAY_TRACK_SPEED_LIMIT)
    {
        speed = GRAY_TRACK_SPEED_LIMIT;
    }
    else if(speed < 0)
    {
        // 循迹时不让单侧轮反转，避免大误差或丢线后原地转圈。
        speed = 0;
    }
    return speed;
}

uint8 gray_track_is_ready(void)
{
    return gray_track_ready;
}

void gray_track_init(void)
{
    uint8 i;

    gray_track_ready = 1;

    for(i = 0; i < 8; i++)
    {
        if(GPIO_MAX == gray_track_pin[i])
        {
            gray_track_ready = 0;
        }
    }

    if(!gray_track_ready)
    {
        // 引脚还没填写时不初始化 GPIO，防止访问无效引脚。
        printf("GRAY_TRACK: pins not configured, fill GRAY_TRACK_OUTx_PIN first.\r\n");
        return;
    }

    for(i = 0; i < 8; i++)
    {
        // 数字灰度 OUT 为数字输入。这里使用上拉输入，若你的模块为推挽输出也可以正常读取。
        gpio_init(gray_track_pin[i], GPI, GPIO_HIGH, GPI_PULL_UP);
    }

    printf("GRAY_TRACK: init ok.\r\n");
}

void gray_track_read(void)
{
    uint8 i;
    int16 sum = 0;
    int16 count = 0;

    if(!gray_track_ready)
    {
        gray_track_lost_line = 1;
        gray_track_error = 0;
        return;
    }

    for(i = 0; i < 8; i++)
    {
        // 先保存原始电平，再转换成“1 表示黑线，0 表示非黑线”，方便现场排查接线和电平方向。
        gray_track_raw[i] = gpio_get_level(gray_track_pin[i]);
        gray_track_value[i] = (gray_track_raw[i] == GRAY_TRACK_BLACK_LEVEL) ? 1 : 0;

        if(gray_track_value[i])
        {
            sum += gray_track_weight[i];
            count++;
        }
    }

    if(count > 0)
    {
        gray_track_error = sum / count;
        gray_track_last_error = gray_track_error;
        gray_track_lost_line = 0;
    }
    else
    {
        // 丢线时保留上一次方向，方便后续扩展成“按上次方向找线”。当前为了安全只停车。
        gray_track_error = gray_track_last_error;
        gray_track_lost_line = 1;
    }
}

void gray_track_run(void)
{
    int16 turn;
    int16 error_delta;
    int16 left_speed;
    int16 right_speed;

    gray_track_read();

    if(!gray_track_ready)
    {
        // 引脚未配置时直接停车，避免访问无效 GPIO 后小车失控。
        Motor_Stop_All();
        return;
    }

    error_delta = gray_track_error - gray_track_last_run_error;
    gray_track_last_run_error = gray_track_error;
    turn = (int16)(GRAY_TRACK_KP * gray_track_error + GRAY_TRACK_KD * error_delta);

    // 差速循迹：黑线偏右 error 为正，左轮加速、右轮减速，使车向右修正。
    left_speed = GRAY_TRACK_BASE_SPEED + turn;
    right_speed = GRAY_TRACK_BASE_SPEED - turn;

    left_speed = gray_track_limit_speed(left_speed);
    right_speed = gray_track_limit_speed(right_speed);

    // 当前底盘电机反向安装：M1 右轮正值前进，M4 左轮负值前进，M2/M3 不使用。
    Motor_Set_Speeds(
        right_speed,
        0,
        0,
        (int16)(-left_speed));
}
