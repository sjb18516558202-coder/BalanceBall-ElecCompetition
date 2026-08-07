#ifndef MOTOR_SET_SPEED_h
#define MOTOR_SET_SPEED_h

#include "zf_common_headfile.h"

/*
 * 电机 PID 参数结构体
 * kp：比例系数
 * ki：积分系数
 * kd：微分系数
 */
typedef struct
{
    float kp;          // 比例系数
    float ki;          // 积分系数
    float kd;          // 微分系数
} PID_t;

/*
 * 函数：Motor_Set_ClosedLoop
 * 作用：设置电机驱动板进入闭环模式。
 * 调用位置：串口初始化完成之后调用一次即可。
 * 调用例子：
 *     uart_init(UART_1, 115200, UART1_TX_A8, UART1_RX_A9);
 *     system_delay_ms(50);
 *     Motor_Set_ClosedLoop();
 */
void Motor_Set_ClosedLoop(void);

/*
 * 函数：Motor_Set_Speeds
 * 作用：一次性设置 4 个电机速度。
 * 参数：
 *     v0：第 1 个电机速度，对应寄存器 0x0000
 *     v1：第 2 个电机速度，对应寄存器 0x0001
 *     v2：第 3 个电机速度，对应寄存器 0x0002
 *     v3：第 4 个电机速度，对应寄存器 0x0003
 * 调用例子：
 *     Motor_Set_Speeds(100, 100, 100, 100);     // 四个电机同速
 *     Motor_Set_Speeds(0, 20, 0, 0);            // 只让第 2 个电机速度为 20
 *     Motor_Set_Speeds(100, -100, 100, -100);   // 正负号可用于控制方向
 */
void Motor_Set_Speeds(int16_t v0, int16_t v1, int16_t v2, int16_t v3);

/*
 * 函数：Motor_Set_One_Speed
 * 作用：单独设置某一个电机速度，其他电机保持上一次速度。
 * 参数：
 *     motor_id：电机编号，从 0 开始，0/1/2/3 分别代表第 1/2/3/4 个电机
 *     speed：目标速度，正负号表示方向，实际最大值要看你的驱动板协议
 * 调用例子：
 *     Motor_Set_One_Speed(0, 100);    // 第 1 个电机速度设为 100
 *     Motor_Set_One_Speed(1, -100);   // 第 2 个电机速度设为 -100
 */
void Motor_Set_One_Speed(uint8_t motor_id, int16_t speed);

/*
 * 函数：Motor_Set_One_Speed_By_Number
 * 作用：单独设置某一个电机速度，其他电机保持上一次速度。
 * 参数：
 *     motor_num：电机编号，从 1 开始，1/2/3/4 分别代表第 1/2/3/4 个电机
 *     speed：目标速度
 * 调用例子：
 *     Motor_Set_One_Speed_By_Number(1, 100);  // 第 1 个电机速度设为 100
 *     Motor_Set_One_Speed_By_Number(2, 20);   // 第 2 个电机速度设为 20
 */
void Motor_Set_One_Speed_By_Number(uint8_t motor_num, int16_t speed);

/*
 * 函数：Motor_Stop_All
 * 作用：停止 4 个电机。
 * 调用例子：
 *     Motor_Stop_All();
 */
void Motor_Stop_All(void);

/*
 * 函数：Motor_Get_One_Speed
 * 作用：读取本程序最近一次发送的目标速度缓存。
 * 注意：这不是编码器反馈速度，只是程序内部保存的目标速度。
 * 调用例子：
 *     int16_t speed = Motor_Get_One_Speed(0);  // 读取第 1 个电机目标速度
 */
int16_t Motor_Get_One_Speed(uint8_t motor_id);

/*
 * 函数：Modbus_ParseFrame
 * 作用：解析电机驱动板返回的 Modbus 数据帧。
 * 调用位置：通常放在串口接收中断或串口接收处理函数中。
 * 调用例子：
 *     Modbus_ParseFrame(rx_data);
 */
void Modbus_ParseFrame(uint8_t data);

/*
 * 编码器极性取反函数
 * 作用：如果某个电机速度给正数但反馈方向不对，可以调用对应函数调整编码器极性。
 * 调用例子：
 *     Motor_Set_Enc1_A();  // 修改 A/第 1 路编码器极性，具体对应关系以驱动板协议为准
 *     Motor_Set_Enc1_B();
 *     Motor_Set_Enc1_C();
 *     Motor_Set_Enc1_D();
 */
void Motor_Set_Enc1_A(void);
void Motor_Set_Enc1_B(void);
void Motor_Set_Enc1_C(void);
void Motor_Set_Enc1_D(void);

/*
 * 函数：Motor_Set_KP_KI_KD
 * 作用：一次性设置 4 个电机的 PID 参数。
 * 注意：函数内部会把 kp/ki/kd 乘以 1000 后发送给驱动板。
 * 调用例子：
 *     PID_t m1 = {1.2f, 0.1f, 0.0f};
 *     PID_t m2 = {1.2f, 0.1f, 0.0f};
 *     PID_t m3 = {1.2f, 0.1f, 0.0f};
 *     PID_t m4 = {1.2f, 0.1f, 0.0f};
 *     Motor_Set_KP_KI_KD(&m1, &m2, &m3, &m4);
 */
void Motor_Set_KP_KI_KD(PID_t *Motor1, PID_t *Motor2, PID_t *Motor3, PID_t *Motor4);

#endif
