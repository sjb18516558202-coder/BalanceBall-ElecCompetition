#ifndef MOTOR_CRC_h
#define MOTOR_CRC_h

#include "zf_common_headfile.h"

/*
 * 函数：CRC16
 * 作用：计算 Modbus RTU 通信帧的 CRC16 校验值。
 * 调用位置：一般不需要在 main.c 中手动调用，Motor_Set_Speeds() 等发送函数内部已经调用。
 * 参数：
 *     puchMsg：需要计算 CRC 的数据缓冲区首地址
 *     usDataLen：参与 CRC 计算的数据长度，不包含最后的 CRC 两个字节
 * 返回值：
 *     16 位 CRC 校验值。发送 Modbus 帧时，一般低字节在前，高字节在后。
 * 调用例子：
 *     uint16_t crc = CRC16(frame, idx);
 *     frame[idx++] = crc & 0xFF;
 *     frame[idx++] = (crc >> 8) & 0xFF;
 */
unsigned short CRC16(uint8 *puchMsg, unsigned short usDataLen);

#endif
