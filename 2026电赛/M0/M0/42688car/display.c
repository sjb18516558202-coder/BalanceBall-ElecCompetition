#include "display.h"

#include <stdio.h>

#include "control.h"
#include "icm42688.h"
#include "odometry.h"
#include "oled.h"
#include "uart_comm.h"

/*
 * OLED 状态显示层：
 * 1. display.c 只负责把小车状态整理成字符串。
 * 2. oled.c 负责真正的屏幕通信。
 * 3. 每行先写空格清屏，再写新内容，避免旧字符残留。
 */

static void display_line(uint8_t row, const char *text)
{
    OLED_ShowString(0U, row, "                ", 8U);
    OLED_ShowString(0U, row, text, 8U);
}

void Display_Init(void)
{
    OLED_Init();
    OLED_Clear();
    display_line(0U, "42688car");
    display_line(1U, "OLED OK");
}

void Display_Update(void)
{
    char line[22];
    CarControlState_t state = Car_GetControlState();
    const ICM42688_Data_t *imu = ICM42688_GetData();
    uint16_t uart0_bytes = CarUart_Available(CAR_UART_PORT_0);
    uint16_t uart1_bytes = CarUart_Available(CAR_UART_PORT_1);
    uint16_t uart2_bytes = CarUart_Available(CAR_UART_PORT_2);

    snprintf(line, sizeof(line), "L%4ld R%4ld",
             (long) state.measured_left_speed,
             (long) state.measured_right_speed);
    display_line(0U, line);

    snprintf(line, sizeof(line), "P%4d %4d",
             state.left_pwm, state.right_pwm);
    display_line(1U, line);

    snprintf(line, sizeof(line), "Y%4d A%s",
             (int) state.current_angle_deg,
             state.angle_loop_enable ? "ON " : "OFF");
    display_line(2U, line);

    snprintf(line, sizeof(line), "X%4d Y%4d",
             (int) (g_carOdometry.x_m * 100.0f),
             (int) (g_carOdometry.y_m * 100.0f));
    display_line(3U, line);

    snprintf(line, sizeof(line), "D%5dcm",
             (int) (g_carOdometry.distance_m * 100.0f));
    display_line(4U, line);

    snprintf(line, sizeof(line), "IMU %s 0x%02X",
             state.imu_ready ? "OK " : "ERR", imu->who_am_i);
    display_line(5U, line);

    snprintf(line, sizeof(line), "U0%03u U1%03u",
             uart0_bytes, uart1_bytes);
    display_line(6U, line);

    snprintf(line, sizeof(line), "U2%03u T%3dC",
             uart2_bytes, (int) imu->temperature_c);
    display_line(7U, line);
}
