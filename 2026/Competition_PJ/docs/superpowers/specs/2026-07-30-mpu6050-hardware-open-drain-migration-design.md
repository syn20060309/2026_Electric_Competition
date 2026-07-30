# MPU6050 硬件开漏 GPIO 迁移设计

## 目标

将 MPU6050 软件 I2C 从 PA24/PA25 的普通 GPIO 软件模拟开漏迁移到
PA0/PA1 的硬件开漏 GPIO，解决总线释放后无法可靠拉高、读取数据始终为零
的问题。保留现有软件 I2C 时序、DMP、加速度任务、OLED、巡线和比赛逻辑。

## 引脚与电气配置

- PA0：MPU6050 SCL。
- PA1：MPU6050 SDA。
- 两个引脚在 `empty.syscfg` 中配置为 `5V Tolerant Open Drain`，
  即 `ioStructure = "OD"`。
- 两个引脚的方向为输出，初始值为 `SET`，上电后总线处于释放状态。
- PA0/PA1仍通过外部上拉电阻连接到3.3V，不上拉到5V。
- PA24/PA25不再分配给MPU6050。

现有资源保持：

- 八路巡线：硬件I2C0，SDA=PA28、SCL=PA31。
- OLED：硬件I2C1，SDA=PA16、SCL=PA15。
- K1、UART、电机、蜂鸣器、定时器和LED引脚不变。

## GPIO操作

删除 `mpu6050_bus.c/.h` 软件开漏封装。旧MPU6050驱动宏直接操作
SysConfig生成的GPIO资源：

```c
#define SDA_OUT() ((void) 0)
#define SDA_IN()  ((void) 0)

#define SDA_GET() \
    ((DL_GPIO_readPins(MPU6050_PORT, MPU6050_SDA_PIN) & \
      MPU6050_SDA_PIN) != 0U)

#define SDA(x) \
    ((x) ? DL_GPIO_setPins(MPU6050_PORT, MPU6050_SDA_PIN) : \
           DL_GPIO_clearPins(MPU6050_PORT, MPU6050_SDA_PIN))

#define SCL(x) \
    ((x) ? DL_GPIO_setPins(MPU6050_PORT, MPU6050_SCL_PIN) : \
           DL_GPIO_clearPins(MPU6050_PORT, MPU6050_SCL_PIN))
```

硬件开漏模式下：

- 写0时GPIO主动把线路拉低。
- 写1时开漏输出晶体管关闭，由外部3.3V上拉电阻把线路拉高。
- SDA读取阶段不再切换GPIO方向；输出保持硬件开漏，写1后即可读取从机电平。

## 初始化错误处理

当前 `MPU6050_Startup()` 忽略 `MPU6050_Init()` 返回值，并且
`mpu_dmp_init()` 在底层 `mpu_init()` 失败时错误返回0。这会让无效总线
进入主循环并显示假零值。

迁移后：

- `MPU6050_Startup()` 只有在MPU6050初始化和DMP初始化均成功时才返回。
- 任一阶段失败时输出 `dmp error`、延时200ms并重新执行完整初始化。
- `mpu_dmp_init()` 在底层 `mpu_init()` 失败时返回明确的非零错误码。
- MPU6050不存在、接线错误或总线无法释放时，小车不进入正常主循环。

## 保持不变的功能

- 软件I2C的起始、停止、ACK、字节发送和字节读取时序保持不变。
- 加速度每30ms读取，OLED每100ms刷新。
- OLED同时显示AX、AY和比赛时间。
- 不增加加速度串口周期输出。
- `LineWalking()`继续在每轮主循环执行。
- `IRR_SPEED`、PID、A点自动停车、计时和里程任务逻辑不变。
- 不手动修改SysConfig生成的 `ti_msp_dl_config.c/.h`。

## 测试与验证

- 更新GPIO宏主机测试：
  - `SCL(0)`和`SDA(0)`清除输出锁存位。
  - `SCL(1)`和`SDA(1)`置位输出锁存位。
  - `SDA_IN()`不关闭输出。
  - `SDA_GET()`返回实际输入电平。
- 更新启动测试，验证MPU初始化失败和DMP初始化失败都会每200ms重试。
- SysConfig CLI必须生成成功，并确认PA0/PA1为硬件OD。
- TI Arm Clang强制全量编译和链接必须无新增错误或警告。
- 实物上电后测量SCL/SDA空闲电压约为3.3V。
- 旋转模块90°时，对应X或Y轴应显示接近正负1.000g。

