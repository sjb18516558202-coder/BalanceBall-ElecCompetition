# 电机函数调用教程（project/code）

## 1. 初始化顺序

电机相关函数已经通过 `zf_common_headfile.h` 间接包含了 `motorsetspeed.h`，所以在 `main.c` 里一般不用额外 include。

推荐初始化顺序：

```c
clock_init(SYSTEM_CLOCK_80M);                         // 系统时钟初始化，必须保留
debug_init();                                         // 调试串口初始化
system_delay_ms(100);
uart_init(UART_1, 115200, UART1_TX_A8, UART1_RX_A9);  // 电机驱动板串口初始化
system_delay_ms(50);
Motor_Set_ClosedLoop();                               // 设置闭环模式
system_delay_ms(50);
Motor_Stop_All();                                     // 上电先停止全部电机
```

## 2. 同时设置 4 个电机速度

```c
Motor_Set_Speeds(v0, v1, v2, v3);
```

参数对应关系：

| 参数 | 对应电机 | 写入寄存器 |
|---|---|---|
| v0 | 第 1 个电机 | 0x0000 |
| v1 | 第 2 个电机 | 0x0001 |
| v2 | 第 3 个电机 | 0x0002 |
| v3 | 第 4 个电机 | 0x0003 |

例子：

```c
Motor_Set_Speeds(100, 100, 100, 100);      // 四个电机同速正转
Motor_Set_Speeds(0, 20, 0, 0);             // 只让第 2 个电机速度为 20
Motor_Set_Speeds(100, -100, 100, -100);    // 正负号相反，可用于测试转向
Motor_Set_Speeds(0, 0, 0, 0);              // 全部停止
```

## 3. 单独设置某一个电机速度

### 写法 A：编号从 0 开始

```c
Motor_Set_One_Speed(0, 100);    // 第 1 个电机速度设为 100
Motor_Set_One_Speed(1, 20);     // 第 2 个电机速度设为 20
Motor_Set_One_Speed(2, -100);   // 第 3 个电机速度设为 -100
Motor_Set_One_Speed(3, 0);      // 第 4 个电机停止
```

### 写法 B：编号从 1 开始，推荐平时用这个

```c
Motor_Set_One_Speed_By_Number(1, 100);    // 第 1 个电机速度设为 100
Motor_Set_One_Speed_By_Number(2, 20);     // 第 2 个电机速度设为 20
Motor_Set_One_Speed_By_Number(3, -100);   // 第 3 个电机速度设为 -100
Motor_Set_One_Speed_By_Number(4, 0);      // 第 4 个电机停止
```

说明：

- 单独设置某个电机时，其他电机会保持上一次设置的速度。
- 例如上电后缓存是 `{0, 0, 0, 0}`，调用 `Motor_Set_One_Speed_By_Number(2, 20)` 后，实际发送的是 `{0, 20, 0, 0}`。

## 4. 停止全部电机

```c
Motor_Stop_All();
```

等价于：

```c
Motor_Set_Speeds(0, 0, 0, 0);
```

## 5. 读取当前目标速度缓存

```c
int16_t speed1 = Motor_Get_One_Speed(0);  // 读取第 1 个电机目标速度
```

注意：这个函数读到的是程序内部保存的“目标速度”，不是编码器反馈速度。

## 6. 设置闭环模式

```c
Motor_Set_ClosedLoop();
```

建议在串口初始化之后、设置速度之前调用一次。

## 7. 修改编码器极性

如果给正速度后，电机反馈方向和你希望的不一致，可以尝试对应的编码器极性取反函数：

```c
Motor_Set_Enc1_A();
Motor_Set_Enc1_B();
Motor_Set_Enc1_C();
Motor_Set_Enc1_D();
```

具体 A/B/C/D 对应哪一路，要以你的电机驱动板协议或实际测试为准。

## 8. 设置 PID 参数

```c
PID_t m1 = {1.2f, 0.1f, 0.0f};
PID_t m2 = {1.2f, 0.1f, 0.0f};
PID_t m3 = {1.2f, 0.1f, 0.0f};
PID_t m4 = {1.2f, 0.1f, 0.0f};

Motor_Set_KP_KI_KD(&m1, &m2, &m3, &m4);
```

注意：`Motor_Set_KP_KI_KD()` 内部会把 `kp/ki/kd` 乘以 1000 后发送，例如 `1.2f` 会发成 `1200`。

## 9. 当前 main.c 中的示例

当前工程 `project/user/src/main.c` 中已经改成：

```c
Motor_Set_One_Speed_By_Number(2, 20);
```

意思是：只让第 2 个电机速度为 20，其余电机保持 0。

如果要改成四个电机一起转，可以改成：

```c
Motor_Set_Speeds(20, 20, 20, 20);
```

如果要全部停止，可以改成：

```c
Motor_Stop_All();
```
