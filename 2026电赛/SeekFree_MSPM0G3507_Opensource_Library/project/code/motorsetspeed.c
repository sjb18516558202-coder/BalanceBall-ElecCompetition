#include "motorsetspeed.h"
#include "motorcrc.h"


/*
 * 电机速度缓存
 * 说明：
 * 1. Motor_Set_Speeds() 是一次设置 4 个电机速度。
 * 2. Motor_Set_One_Speed() / Motor_Set_One_Speed_By_Number() 只修改某一个电机速度，
 *    其余电机会保持上一次设置的速度，所以这里需要保存 4 个电机的当前速度。
 * 3. 速度使用 int16_t，有符号数可以表示正转和反转。
 */
static int16_t motor_speed_cache[4] = {0, 0, 0, 0};

/*
 * 将 16 位数据按 Modbus 寄存器格式写入发送缓冲区：高字节在前，低字节在后。
 * 注意：速度可能是负数，先转换为 uint16_t，可以保证 -1 会按 0xFFFF 发送。
 */
static void Motor_Frame_Put_U16(uint8_t *frame, uint8_t *idx, uint16_t value)
{
    frame[(*idx)++] = (uint8_t)((value >> 8) & 0xFF);
    frame[(*idx)++] = (uint8_t)(value & 0xFF);
}




void Motor_Set_ClosedLoop(void)
{

		uint8 idx = 0;
		uint8 frame[20];

    frame[idx++] = 0x0A;        // 从站地址
    frame[idx++] = 0x06;        // 功能码：写单个保持寄存器

    frame[idx++] = 0x00;        // 起始寄存器高字节
    frame[idx++] = 0x08;        // 起始寄存器低字节

  
    // 寄存器 1
    frame[idx++] = 0x00;
    frame[idx++] = 0x01;

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;

    // 发送
//    for (uint8_t i = 0; i < idx; i++)
//{
//    HAL_UART_Transmit(&huart1, &frame[i], 1, HAL_MAX_DELAY);
//}
    uart_write_buffer(UART_2, frame, idx);
}



void Motor_Set_Speeds(int16_t v0, int16_t v1, int16_t v2, int16_t v3)
{
    uint8_t idx = 0;
    uint8_t frame[20];

    /*
     * 同时设置 4 个电机速度。
     * 参数对应关系：
     * v0 -> 第 1 个电机，写入寄存器 0x0000
     * v1 -> 第 2 个电机，写入寄存器 0x0001
     * v2 -> 第 3 个电机，写入寄存器 0x0002
     * v3 -> 第 4 个电机，写入寄存器 0x0003
     *
     * 例子：
     * Motor_Set_Speeds(100, 100, 100, 100);    // 四个电机同速正转
     * Motor_Set_Speeds(0, 20, 0, 0);           // 只让第 2 个电机速度为 20
     * Motor_Set_Speeds(100, -100, 100, -100);  // 根据接线可能是原地转向
     */

    /* 同步缓存，后面单独设置某个电机时，其他电机能保持当前速度 */
    motor_speed_cache[0] = v0;
    motor_speed_cache[1] = v1;
    motor_speed_cache[2] = v2;
    motor_speed_cache[3] = v3;

    frame[idx++] = 0x0A;        // 从站地址
    frame[idx++] = 0x10;        // 功能码：写多个保持寄存器

    frame[idx++] = 0x00;        // 起始寄存器高字节
    frame[idx++] = 0x00;        // 起始寄存器低字节，从 0x0000 开始

    frame[idx++] = 0x00;        // 寄存器数量高字节
    frame[idx++] = 0x04;        // 寄存器数量低字节，连续写 4 个寄存器

    frame[idx++] = 0x08;        // 数据字节数 = 4 个寄存器 × 2 字节 = 8

    /*
     * int16_t 可能为负数，先强制转换为 uint16_t 再拆高低字节。
     * 这样 -100 会按照补码格式发送给驱动器。
     */
    Motor_Frame_Put_U16(frame, &idx, (uint16_t)v0);
    Motor_Frame_Put_U16(frame, &idx, (uint16_t)v1);
    Motor_Frame_Put_U16(frame, &idx, (uint16_t)v2);
    Motor_Frame_Put_U16(frame, &idx, (uint16_t)v3);

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;          // Modbus CRC 低字节在前
    frame[idx++] = (crc >> 8) & 0xFF;   // Modbus CRC 高字节在后

    uart_write_buffer(UART_2, frame, idx);
}


void Motor_Set_One_Speed(uint8_t motor_id, int16_t speed)
{
    /*
     * 单独设置某一个电机速度，编号从 0 开始。
     * motor_id = 0 -> 第 1 个电机
     * motor_id = 1 -> 第 2 个电机
     * motor_id = 2 -> 第 3 个电机
     * motor_id = 3 -> 第 4 个电机
     *
     * 调用例子：
     * Motor_Set_One_Speed(0, 100);   // 第 1 个电机速度设为 100
     * Motor_Set_One_Speed(1, -100);  // 第 2 个电机速度设为 -100，方向相反
     * Motor_Set_One_Speed(3, 0);     // 第 4 个电机停止
     */
    if (motor_id >= 4)
    {
        return;     // 编号错误，直接退出，防止数组越界
    }

    motor_speed_cache[motor_id] = speed;

    Motor_Set_Speeds(
        motor_speed_cache[0],
        motor_speed_cache[1],
        motor_speed_cache[2],
        motor_speed_cache[3]
    );
}


void Motor_Set_One_Speed_By_Number(uint8_t motor_num, int16_t speed)
{
    /*
     * 单独设置某一个电机速度，编号从 1 开始，更适合平时调用。
     * motor_num = 1 -> 第 1 个电机
     * motor_num = 2 -> 第 2 个电机
     * motor_num = 3 -> 第 3 个电机
     * motor_num = 4 -> 第 4 个电机
     *
     * 调用例子：
     * Motor_Set_One_Speed_By_Number(1, 100);   // 第 1 个电机速度设为 100
     * Motor_Set_One_Speed_By_Number(2, 20);    // 第 2 个电机速度设为 20
     * Motor_Set_One_Speed_By_Number(4, 0);     // 第 4 个电机停止
     */
    if ((motor_num < 1) || (motor_num > 4))
    {
        return;     // 编号错误，直接退出
    }

    Motor_Set_One_Speed((uint8_t)(motor_num - 1), speed);
}


void Motor_Stop_All(void)
{
    /*
     * 停止全部电机。
     * 调用例子：
     * Motor_Stop_All();
     */
    Motor_Set_Speeds(0, 0, 0, 0);
}


int16_t Motor_Get_One_Speed(uint8_t motor_id)
{
    /*
     * 读取当前缓存中的电机速度。
     * 注意：这里读到的是本程序最近一次发送的目标速度，不是编码器反馈速度。
     *
     * 调用例子：
     * int16_t speed = Motor_Get_One_Speed(0);  // 获取第 1 个电机当前目标速度
     */
    if (motor_id >= 4)
    {
        return 0;
    }

    return motor_speed_cache[motor_id];
}



void Motor_Set_Enc1_A(void)
{

		uint8_t idx = 0;
		uint8_t frame[20];

    frame[idx++] = 0x0A;        // 从站地址
    frame[idx++] = 0x06;        // 功能码：写单个保持寄存器

    frame[idx++] = 0x00;        // 起始寄存器高字节
    frame[idx++] = 0x09;        // 起始寄存器低字节

  
    // 寄存器 1
    frame[idx++] = 0x00;
    frame[idx++] = 0x01;

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;

    // 发送
//   for (uint8_t i = 0; i < idx; i++)
//{
//    HAL_UART_Transmit(&huart1, &frame[i], 1, HAL_MAX_DELAY);
//}
    uart_write_buffer(UART_2, frame, idx);

}


void Motor_Set_Enc1_B(void)
{

		uint8_t idx = 0;
		uint8_t frame[20];

    frame[idx++] = 0x0A;        // 从站地址
    frame[idx++] = 0x06;        // 功能码：写单个保持寄存器

    frame[idx++] = 0x00;        // 起始寄存器高字节
    frame[idx++] = 0x0A;        // 起始寄存器低字节

  
    // 寄存器 1
    frame[idx++] = 0x00;
    frame[idx++] = 0x01;

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;

    // 发送
//    for (uint8_t i = 0; i < idx; i++)
//{
//    HAL_UART_Transmit(&huart1, &frame[i], 1, HAL_MAX_DELAY);
//}
    uart_write_buffer(UART_2, frame, idx);

}

void Motor_Set_Enc1_C(void)
{

		uint8_t idx = 0;
		uint8_t frame[20];

    frame[idx++] = 0x0A;        // 从站地址
    frame[idx++] = 0x06;        // 功能码：写单个保持寄存器

    frame[idx++] = 0x00;        // 起始寄存器高字节
    frame[idx++] = 0x0B;        // 起始寄存器低字节

  
    // 寄存器 1
    frame[idx++] = 0x00;
    frame[idx++] = 0x01;

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;

    // 发送
//    for (uint8_t i = 0; i < idx; i++)
//{
//    HAL_UART_Transmit(&huart1, &frame[i], 1, HAL_MAX_DELAY);
//}
    uart_write_buffer(UART_2, frame, idx);

}

void Motor_Set_Enc1_D(void)
{

		uint8_t idx = 0;
		uint8_t frame[20];

    frame[idx++] = 0x0A;        // 从站地址
    frame[idx++] = 0x06;        // 功能码：写单个保持寄存器

    frame[idx++] = 0x00;        // 起始寄存器高字节
    frame[idx++] = 0x0C;        // 起始寄存器低字节

  
    // 寄存器 1
    frame[idx++] = 0x00;
    frame[idx++] = 0x01;

    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;

    // 发送
//   for (uint8_t i = 0; i < idx; i++)
//{
//    HAL_UART_Transmit(&huart1, &frame[i], 1, HAL_MAX_DELAY);
//}
    uart_write_buffer(UART_2, frame, idx);

}












void Motor_Set_KP_KI_KD(PID_t *Motor1, PID_t *Motor2, PID_t *Motor3, PID_t *Motor4)
{
		uint16_t Kp_Temp;
		uint16_t Ki_Temp;
		uint16_t Kd_Temp;

    uint8_t idx = 0;
		uint8_t frame[34];

    frame[idx++] = 0x0A;        // 从站地址
    frame[idx++] = 0x10;        // 功能码：写多个保持寄存器

    frame[idx++] = 0x00;  // 起始寄存器高字节
		frame[idx++] = 0x15;  // 起始寄存器低字节 (21)
	
    frame[idx++] = 0x00;        // 寄存器数量高字节
    frame[idx++] = 0x0C;        // 寄存器数量低字节（12 个）
	
    frame[idx++] = 0x18;   // 数据字节数 = 12 × 2 = 24


		Kp_Temp = (uint16_t)(Motor1->kp * 1000);
		Ki_Temp = (uint16_t)(Motor1->ki * 1000);
		Kd_Temp = (uint16_t)(Motor1->kd * 1000);

    frame[idx++] = (Kp_Temp >> 8) & 0xFF;
    frame[idx++] = (Kp_Temp >> 0) & 0xFF;


    frame[idx++] = (Ki_Temp >> 8) & 0xFF;
    frame[idx++] = (Ki_Temp >> 0) & 0xFF;

    frame[idx++] = (Kd_Temp >> 8) & 0xFF;
    frame[idx++] = (Kd_Temp >> 0) & 0xFF;


		Kp_Temp = (uint16_t)(Motor2->kp * 1000);
		Ki_Temp = (uint16_t)(Motor2->ki * 1000);
		Kd_Temp = (uint16_t)(Motor2->kd * 1000);

    frame[idx++] = (Kp_Temp >> 8) & 0xFF;
    frame[idx++] = (Kp_Temp >> 0) & 0xFF;


    frame[idx++] = (Ki_Temp >> 8) & 0xFF;
    frame[idx++] = (Ki_Temp >> 0) & 0xFF;

    frame[idx++] = (Kd_Temp >> 8) & 0xFF;
    frame[idx++] = (Kd_Temp >> 0) & 0xFF;



		Kp_Temp = (uint16_t)(Motor3->kp * 1000);
		Ki_Temp = (uint16_t)(Motor3->ki * 1000);
		Kd_Temp = (uint16_t)(Motor3->kd * 1000);

    frame[idx++] = (Kp_Temp >> 8) & 0xFF;
    frame[idx++] = (Kp_Temp >> 0) & 0xFF;


    frame[idx++] = (Ki_Temp >> 8) & 0xFF;
    frame[idx++] = (Ki_Temp >> 0) & 0xFF;

    frame[idx++] = (Kd_Temp >> 8) & 0xFF;
    frame[idx++] = (Kd_Temp >> 0) & 0xFF;


		Kp_Temp = (uint16_t)(Motor4->kp * 1000);
		Ki_Temp = (uint16_t)(Motor4->ki * 1000);
		Kd_Temp = (uint16_t)(Motor4->kd * 1000);

    frame[idx++] = (Kp_Temp >> 8) & 0xFF;
    frame[idx++] = (Kp_Temp >> 0) & 0xFF;


    frame[idx++] = (Ki_Temp >> 8) & 0xFF;
    frame[idx++] = (Ki_Temp >> 0) & 0xFF;

    frame[idx++] = (Kd_Temp >> 8) & 0xFF;
    frame[idx++] = (Kd_Temp >> 0) & 0xFF;












   
    uint16_t crc = CRC16(frame, idx);
    frame[idx++] = crc & 0xFF;
    frame[idx++] = (crc >> 8) & 0xFF;

    // 发送
//for (uint8_t i = 0; i < idx; i++)
//{
//    HAL_UART_Transmit(&huart1, &frame[i], 1, HAL_MAX_DELAY);
//}
    uart_write_buffer(UART_2, frame, idx);

}
