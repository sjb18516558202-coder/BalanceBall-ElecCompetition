# 42688car 代码参考文档

这份文档对应当前工作区里的这版代码，不再对应之前那版带 UART 文本命令控制的小车代码。

当前工程特点：

- 按键只置标志位，不直接控制小车
- 串口模块只保留基础收发接口，不在 `main.c` 里做命令解析
- PID 已经简化成接近原模板工程的写法
- 不接陀螺仪时，也可以先测试电机、编码器和速度环

---

## 1. 工程结构

### 1.1 主文件

- `main.c`
  工程入口，负责初始化、主循环、10ms 控制定时器中断入口。

- `empty.syscfg`
  SysConfig 外设和引脚配置文件。CCS 重新生成工程代码时会用到它。

### 1.2 控制相关

- `control.c / control.h`
  小车控制主逻辑。包括：
  - 左右轮速度环
  - 角度环
  - 每 10ms 一次的控制更新

- `pid.c / pid.h`
  简化后的 PID 模块。

- `motor.c / motor.h`
  电机 PWM 和方向控制。

- `encoder.c / encoder.h`
  编码器读取和速度增量计算。

- `odometry.c / odometry.h`
  里程计计算。

### 1.3 传感器与显示

- `icm42688.c / icm42688.h`
  ICM42688 陀螺仪/加速度计 SPI 驱动。

- `display.c / display.h`
  OLED 状态显示组织层。

- `oled.c / oled.h`
  OLED 底层驱动。

### 1.4 其他模块

- `key.c / key.h`
  三个按键的消抖扫描，只置按键标志位。

- `uart_comm.c / uart_comm.h`
  三路 UART 的底层发送、接收、缓存。

- `clock.c / clock.h`
  SysTick 1ms 计时和阻塞延时。

- `app_config.h`
  全工程重要参数集中定义文件。

---

## 2. 重要接口

## 2.1 main.c

### `int main(void)`

作用：

- 初始化全部外设
- 初始化 SysTick
- 初始化串口、OLED、陀螺仪、按键、控制模块
- 打开 10ms 定时器中断
- 在主循环里等待按键逻辑和 OLED 刷新

### `void CONTROL_TIMER_INST_IRQHandler(void)`

作用：

- 10ms 定时器中断服务函数
- 每 10ms 进入一次
- 内部调用：
  - `Car_ControlUpdate10ms()`
  - `CarKey_Update10ms()`
  - OLED 刷新调度

---

## 2.2 control.h / control.c

### `void Car_ControlInit(void)`

作用：

- 初始化左右轮速度环 PID
- 初始化角度环 PID
- 初始化电机
- 初始化编码器
- 清零里程计

### `void Car_MotorControl(int16_t left_speed, int16_t right_speed, bool angle_loop_enable, float target_angle_deg)`

作用：

- 给控制模块设置目标命令

参数说明：

- `left_speed`
  左轮目标速度，单位是“每 10ms 编码器 tick”

- `right_speed`
  右轮目标速度，单位同上

- `angle_loop_enable`
  是否开启角度环
  - `false`：只跑左右轮速度环
  - `true`：在速度环基础上再加角度修正

- `target_angle_deg`
  目标航向角，单位是度

### `void Car_ControlUpdate10ms(void)`

作用：

- 每 10ms 执行一次完整控制流程

流程顺序：

1. 读取编码器增量
2. 读取 IMU 数据
3. 更新里程计
4. 如果角度环开启且 IMU 正常，则先计算角度修正量
5. 根据角度修正量得到左右轮新的目标速度
6. 左右轮分别进入速度环
7. 输出左右轮 PWM
8. 更新状态结构体 `g_state`

### `void Car_Stop(void)`

作用：

- 让小车立即停止
- 清零速度环和角度环状态

### `CarControlState_t Car_GetControlState(void)`

作用：

- 返回当前控制状态快照
- 供 OLED 或调试查看使用

---

## 2.3 pid.h / pid.c

当前 PID 已经精简成最常用的几个接口。

### `void SpeedPID_Init(PIDController_t *pid)`

作用：

- 初始化速度环参数
- 使用 `app_config.h` 里的原工程参数

### `void AnglePID_Init(PIDController_t *pid)`

作用：

- 初始化角度环参数
- 使用 `app_config.h` 里的原工程参数

### `void PID_Reset(PIDController_t *pid)`

作用：

- 清零误差
- 清零积分
- 清零输出

### `int16_t SpeedPID_Update(PIDController_t *pid, int16_t target_speed, int32_t current_speed)`

作用：

- 更新一次速度环
- 返回新的 PWM 输出

写法说明：

当前使用的是增量式 PI，尽量按照你原模板工程的思路来写：

- 本次误差 = 目标速度 - 当前速度
- 输出增量 = `Kp * (本次误差 - 上次误差) + Ki * 本次误差`
- 最终 PWM = 上一次 PWM + 本次输出增量

### `int16_t AnglePID_Update(PIDController_t *pid, float target_deg, float current_deg, float gyro_z_dps)`

作用：

- 更新一次角度环
- 返回左右轮差速修正量

说明：

- 比例项：看当前角度误差
- 积分项：消除长期偏差
- 微分项：直接使用 Z 轴角速度

### `float PID_WrapDegrees(float angle_deg)`

作用：

- 把角度限制到 `-180 ~ 180`
- 避免跨 0 度时误差异常变大

---

## 2.4 key.h / key.c

### 三个按键标志位

- `key1_flag`
- `key2_flag`
- `key3_flag`

逻辑：

- 按键按下并松手后，对应标志位变成 `1`
- 主循环里自己判断
- 处理完以后自己手动清 `0`

示例：

```c
if (key1_flag) {
    key1_flag = 0;
    // 这里写按键1要做的事
}
```

### `void CarKey_Init(void)`

作用：

- 初始化按键消抖状态
- 清零三个按键标志位

### `void CarKey_Update10ms(void)`

作用：

- 每 10ms 扫描一次按键
- 完成消抖
- 只负责把 `key1_flag/key2_flag/key3_flag` 置位

---

## 2.5 motor.h / motor.c

### `void Motor_Init(void)`

作用：

- 初始化电机 PWM
- 初始化电机方向引脚

### `void Motor_SetPwm(int16_t left_pwm, int16_t right_pwm)`

作用：

- 直接设置左右轮 PWM

说明：

- 正数：正转
- 负数：反转
- 0：停止

这个函数非常适合最开始单独测试电机。

### `void Motor_Stop(void)`

作用：

- 让左右轮停止

---

## 2.6 encoder.h / encoder.c

### `void Encoder_Init(void)`

作用：

- 启动左右编码器计数

### `void Encoder_Reset(void)`

作用：

- 清零软件记录的速度增量和累计值

### `void Encoder_Update(void)`

作用：

- 读取本周期左右编码器计数变化量

### `EncoderState_t Encoder_GetState(void)`

作用：

- 返回编码器状态

常看字段：

- `left_delta`
- `right_delta`
- `left_total`
- `right_total`

---

## 2.7 icm42688.h / icm42688.c

### `void ICM42688_Init(void)`

作用：

- 初始化 ICM42688
- 读取 `WHO_AM_I`
- 配置量程和工作模式

### `bool ICM42688_IsReady(void)`

作用：

- 判断陀螺仪是否识别成功

### `bool ICM42688_ReadData(ICM42688_Data_t *out)`

作用：

- 读取一次 IMU 数据
- 返回 `true` 表示成功
- 返回 `false` 表示当前 IMU 不可用

### `void ICM42688_SetYaw(float yaw_deg)`

作用：

- 手动设置当前航向角
- 一般用于上电归零

### 不接陀螺仪时是否能跑

可以。

当前代码里：

- 如果 `WHO_AM_I` 读不到正确值
- `g_imu.present` 就会是 `false`
- 后面 `ICM42688_ReadData()` 会直接返回 `false`
- 控制层就不会进入角度环

所以：

- 不接陀螺仪，可以测试电机
- 不接陀螺仪，可以测试编码器
- 不接陀螺仪，可以测试速度环
- 不能测试角度环

---

## 2.8 uart_comm.h / uart_comm.c

当前串口模块只保留底层接口，没有在 `main.c` 里写串口命令解析。

### `void CarUart_Init(void)`

作用：

- 初始化三路串口接收缓冲区
- 打开对应串口中断

### `void CarUart_SendByte(...)`

作用：

- 发送 1 个字节

### `void CarUart_SendBuffer(...)`

作用：

- 发送一段数据

### `void CarUart_SendString(...)`

作用：

- 发送字符串

### `uint16_t CarUart_Available(...)`

作用：

- 查看当前缓冲区里还有多少未读字节

### `uint16_t CarUart_Read(...)`

作用：

- 从环形缓冲区里读出字节流

### `bool CarUart_ReadLine(...)`

作用：

- 读一整行，直到 `\n`

说明：

虽然现在 `main.c` 里没用它，但你以后自己写串口协议时还可以继续用。

---

## 2.9 display.h / display.c

### `void Display_Init(void)`

作用：

- 初始化 OLED
- 显示启动信息

### `void Display_Update(void)`

作用：

- 刷新 OLED 页面

当前 OLED 主要显示：

- 左右轮速度
- 左右轮 PWM
- 当前角度
- 里程
- IMU 状态
- 串口缓存字节数

---

## 2.10 clock.h / clock.c

### `void SysTick_Init(void)`

作用：

- 把 Cortex-M0 的 SysTick 配成每 1ms 中断一次

### `void SysTick_Handler(void)`

作用：

- SysTick 的中断服务函数
- 每次进来代表过去了 1ms

### `int mspm0_delay_ms(unsigned long num_ms)`

作用：

- 阻塞延时

说明：

- 阻塞就是“没等够时间就不会返回”
- 用之前必须保证 SysTick 正常工作

---

## 3. 重要参数

所有重要参数都集中在 [app_config.h](/C:/Users/ASUS/Desktop/diansai/42688car/app_config.h)。

### 控制周期

- `CAR_CONTROL_PERIOD_MS = 10`
- `CAR_CONTROL_PERIOD_S = 0.01`

### PWM

- `CAR_PWM_MAX = 1600`

### 速度环 PID

- `CAR_SPEED_KP = 50`
- `CAR_SPEED_KI = 18`
- `CAR_SPEED_KD = 0`

### 角度环 PID

- `CAR_ANGLE_KP = 0.39`
- `CAR_ANGLE_KI = 0.01`
- `CAR_ANGLE_KD = 0.03`
- `CAR_ANGLE_OUTPUT_LIMIT = 40`
- `CAR_ANGLE_INTEGRAL_LIMIT = 100`

### 编码器和里程计

- `CAR_WHEEL_DIAMETER_M`
- `CAR_WHEEL_TRACK_M`
- `CAR_ENCODER_TICKS_PER_REV`
- `CAR_RIGHT_ENCODER_EDGE_SCALE`
- `CAR_LEFT_ENCODER_DIR_INVERT`
- `CAR_RIGHT_ENCODER_DIR_INVERT`
- `CAR_RIGHT_EDGE_TIMER_COUNTS_DOWN`

### IMU

- `ICM42688_WHO_AM_I_EXPECTED = 0x47`
- `ICM42688_ACCEL_LSB_PER_G`
- `ICM42688_GYRO_LSB_PER_DPS`
- `ICM42688_TEMP_LSB_PER_C`
- `ICM42688_TEMP_OFFSET_C`

---

## 4. 当前重要引脚

详细配置以 `empty.syscfg` 和 `Debug/ti_msp_dl_config.h` 为准。

### 电机

- 左轮方向：PA22 / PA12
- 右轮方向：PA17 / PA15
- 左轮 PWM：PB8
- 右轮 PWM：PB20

### 编码器

- 左编码器 QEI：PA29 / PA30
- 右编码器：
  - A 相计数：PB2
  - B 相方向判断：PB3

### 按键

- 按键1：PB21
- 按键2：PB15
- 按键3：PB19

### OLED

- SCL：PB12
- SDA：PB13

### ICM42688

- CS：PA2
- SPI SCLK：PA6
- SPI MOSI：PA5
- SPI MISO：PA4

### UART

- UART0：PA10 TX / PA11 RX
- UART1：PB6 TX / PB7 RX
- UART2：PB17 TX / PB16 RX

---

## 5. 正确上手和调试顺序

这个顺序很重要，尽量不要一上来就直接测整车闭环。

## 第 1 步：先确认代码能下载、能运行

上电后先看：

- 程序能不能正常下载
- OLED 是否亮
- 是否能进入主循环

如果这一步不通，先别碰 PID。

---

## 第 2 步：先测电机，不要先测 PID

最先测的是 `Motor_SetPwm()`，不是 `Car_MotorControl()`。

例如在主循环里临时写：

```c
Motor_SetPwm(300, 300);
```

先看这些最基础的问题：

- 左右轮转不转
- 左右轮方向对不对
- 电机驱动接线对不对
- PWM 是否生效

如果方向反了：

- 先查接线
- 再查 `motor.c`

---

## 第 3 步：再测编码器

电机能转后，再看编码器值。

重点观察：

- `encoder.left_delta`
- `encoder.right_delta`
- `encoder.left_total`
- `encoder.right_total`

需要确认：

- 轮子转时计数有没有变化
- 左右轮计数方向对不对

如果方向不对，优先改这些参数：

- `CAR_LEFT_ENCODER_DIR_INVERT`
- `CAR_RIGHT_ENCODER_DIR_INVERT`
- `CAR_RIGHT_EDGE_TIMER_COUNTS_DOWN`

---

## 第 4 步：不接陀螺仪，先测速度环

这一步先不要开角度环。

调用方式：

```c
Car_MotorControl(10, 10, false, 0.0f);
```

第三个参数一定先写 `false`。

这样只测试：

- 左轮速度环
- 右轮速度环

不测试：

- 角度环

如果此时速度不稳定，再去调：

- `CAR_SPEED_KP`
- `CAR_SPEED_KI`

---

## 第 5 步：陀螺仪到了以后，再测角度环

等 ICM42688 接上后，再开：

```c
Car_MotorControl(10, 10, true, 0.0f);
```

这时再调：

- `CAR_ANGLE_KP`
- `CAR_ANGLE_KI`
- `CAR_ANGLE_KD`

如果 IMU 没接好，OLED 上会看到：

- `IMU ERR`

这时不要调角度环，先查 IMU 接线和 SPI。

---

## 第 6 步：最后再写按键逻辑

按键最好放到最后接，因为它只是控制入口，不是底层功能。

你在 `while(1)` 里这样写就行：

```c
if (key1_flag) {
    key1_flag = 0;
    // 写按键1动作
}

if (key2_flag) {
    key2_flag = 0;
    // 写按键2动作
}

if (key3_flag) {
    key3_flag = 0;
    // 写按键3动作
}
```

---

## 6. 现在这版代码最适合怎么用

如果你现在陀螺仪还没到，推荐这样用：

1. 下载程序
2. 先看 OLED 和主循环是否正常
3. 用 `Motor_SetPwm()` 测左右轮
4. 再看编码器增量是否正常
5. 再用 `Car_MotorControl(..., false, 0.0f)` 测速度环
6. 陀螺仪到了以后，再开角度环

---

## 7. 说明

这份文档对应当前这版代码。

如果后面你又改了：

- 按键逻辑
- PID 写法
- 串口协议
- 显示内容
- 引脚分配

那这份文档也应该跟着一起更新，不然很容易“代码已经改了，文档还是旧的”。
