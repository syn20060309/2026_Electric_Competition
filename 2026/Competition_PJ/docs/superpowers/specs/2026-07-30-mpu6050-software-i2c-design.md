# MPU6050 软件 I2C 接入设计

## 目标

在现有 MSPM0G3507 小车工程中接入 MPU6050 和 DMP，保留现有 OLED、八路巡线、电机、按键、蜂鸣器及比赛逻辑。

## 引脚与电气方式

- PA24：MPU6050 SCL。
- PA25：MPU6050 SDA。
- 两个引脚在 `empty.syscfg` 中配置为普通 GPIO 输出，初始值为 `SET`。
- PA24、PA25 不支持 MSPM0G3507 的硬件 `5V Tolerant Open Drain` I/O 结构，因此由软件模拟开漏：
  - 输出 0：清零输出锁存器并使能输出。
  - 输出 1：关闭输出驱动，由外部 3.3 V 上拉电阻将总线拉高。
- SDA 读取时保持输出驱动关闭。
- 总线必须上拉到 3.3 V，不能上拉到 5 V。

## 软件结构

- `empty.syscfg` 新增名为 `MPU6050` 的 GPIO 组以及 `SCL`、`SDA` 两个引脚。
- `BSP/MPU6050/bsp_mpu6050.c/.h` 提供软件 I2C 和 MPU6050 寄存器访问。
- `BSP/eMPL/` 提供 DMP 驱动。
- `empty.c` 在串口初始化完成后调用 `MPU6050_Init()`，随后循环调用 `mpu_dmp_init()`，失败时输出 `dmp error` 并延时 200 ms 重试。
- CCS 工程增加 `BSP/MPU6050` 和 `BSP/eMPL` 头文件路径。

## 启动与故障行为

- MPU6050 不存在或 DMP 初始化失败时，程序停留在 DMP 重试循环，不启动小车。
- DMP 初始化成功后继续现有电机、定时器、OLED 与主循环初始化。
- 不在本阶段把姿态角接入巡线或电机控制。

## 验证

- 主机测试验证软件开漏的“拉低/释放”行为和 DMP 重试启动流程。
- SysConfig 生成必须成功且 PA24/PA25 不与现有资源冲突。
- CCS/TI Arm Clang 全量编译必须成功。
- 实物验证 MPU6050 WHO_AM_I、DMP 初始化、SDA/SCL 电平和现有巡线/OLED 功能。
