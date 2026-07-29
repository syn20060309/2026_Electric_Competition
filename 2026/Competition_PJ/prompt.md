# MSPM0G3507 小车工程增加 OLED 计时显示功能

## 一、项目位置

请直接检查并修改以下工程：

```text
E:\my Desktop\2026_Electric_Competition\2026\Competition_PJ
```

这是一个基于 **MSPM0G3507** 的 TI CCS / SysConfig 工程。

当前小车已经能够正常完成巡线，现有功能包括：

- 八路红外循迹
- 电机控制
- 启动按键
- 定时器
- 串口调试
- 蜂鸣器和 LED

请先完整阅读工程，重点检查：

```text
empty.c
empty.syscfg
BSP/Key/
BSP/Timer/
BSP/Eight_Tracking/
BSP/Motor/
BSP/delay.c
BSP/usart.c
ti_msp_dl_config.c
ti_msp_dl_config.h
```

理解当前程序的按键启动逻辑、巡线启动标志和计时方式后，再进行修改。

---

## 二、开发目标

为小车增加一个 I²C OLED 显示模块，实现：

> 按下小车启动按键时，计时系统从 0 开始计时，并在 OLED 上实时显示已经运行的时间。

OLED 使用硬件 I²C 通信。

---

## 三、OLED 硬件连接

OLED 为常见 4 针 I²C OLED，接口为：

```text
GND
VDD
SCK
SDA
```

连接关系固定如下：

| OLED 引脚 | MSPM0G3507 引脚 | 功能 |
|---|---|---|
| GND | GND | 电源地 |
| VDD | 3.3V | 电源 |
| SCK | PA1 | I2C0_SCL |
| SDA | PA0 | I2C0_SDA |

必须使用：

```text
PA1 -> I2C0_SCL
PA0 -> I2C0_SDA
```

禁止改用软件模拟 I²C。

---

## 四、引脚配置约束

1. 使用 MSPM0G3507 的硬件 `I2C0`。
2. 在 `empty.syscfg` 中配置 I2C Controller。
3. I²C 初始速率使用 `100 kHz`，确认工作稳定后可设置为 `400 kHz`。
4. PA0 和 PA1 专门用于 OLED：
   - PA0：I2C0_SDA
   - PA1：I2C0_SCL
5. 不得修改其他已有外设的引脚分配。
6. 如果 PA0、PA1 当前被 UART0 或其他外设占用：
   - 只处理 PA0、PA1 的复用冲突；
   - 不要擅自修改其他引脚；
   - 在最终说明中明确指出冲突和处理方式。
7. 保持现有电机、循迹、按键、蜂鸣器、LED 等硬件连接不变。

---

## 五、OLED 驱动要求

优先按照以下 OLED 参数开发：

```text
控制芯片：SSD1306 或兼容芯片
分辨率：128 × 64
通信接口：I²C
默认 7 位地址：0x3C
```

但需要将 OLED 地址定义为宏，方便以后修改：

```c
#define OLED_I2C_ADDRESS 0x3C
```

注意检查 MSPM0 DriverLib 的 I²C API 使用的是：

- 7 位地址；
- 还是已经左移后的地址。

禁止因为地址格式错误而重复左移。

建议新增独立 OLED 驱动目录：

```text
BSP/OLED/
├── oled.c
├── oled.h
├── oled_font.c
└── oled_font.h
```

至少提供以下接口：

```c
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_ShowChar(uint8_t x, uint8_t y, char ch);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str);
void OLED_ShowTime(uint32_t elapsed_ms);
```

如果采用显存缓存方式，缓存大小应为：

```c
uint8_t OLED_GRAM[128][8];
```

或者使用等价的一维 1024 字节缓存。

底层 I²C 写接口建议包括：

```c
bool OLED_WriteCommand(uint8_t command);
bool OLED_WriteData(const uint8_t *data, uint16_t length);
```

需要增加必要的：

- I²C BUSY 状态检查；
- 超时保护；
- 错误返回值；
- 防止程序因 OLED 未连接而永久卡死。

禁止使用无超时的死循环，例如：

```c
while (DL_I2C_getControllerStatus(...) & BUSY);
```

应当增加合理超时。

---

## 六、计时功能要求

请优先复用工程现有的计时模块和 `Get_Time()`，不要重复创建功能相同的定时器。

当前主循环中已经存在类似代码：

```c
uint32_t now = Get_Time();
```

请先确认：

1. `Get_Time()` 的单位是否为毫秒；
2. 系统上电后是否一直累计；
3. 是否已有按键启动时间记录变量；
4. 是否已有小车启动标志，例如：

```c
g_LinePortal_flag
```

实现计时逻辑：

```text
系统上电
    ↓
OLED 初始化
    ↓
等待启动按键
    ↓
按键有效按下
    ↓
记录启动时刻
    ↓
时间从 0 开始显示
    ↓
小车正常巡线
```

推荐采用时间差方式，而不是在主循环中手动累加：

```c
elapsed_ms = Get_Time() - start_time_ms;
```

建议变量：

```c
static uint32_t start_time_ms = 0;
static uint32_t elapsed_time_ms = 0;
static uint32_t last_oled_update_ms = 0;
static bool timing_running = false;
```

启动按键有效触发时：

```c
start_time_ms = Get_Time();
elapsed_time_ms = 0;
timing_running = true;
```

必须在按键完成消抖、确认是一次有效按下事件后启动计时。

禁止按键持续按住时反复清零计时。

---

## 七、OLED 显示内容

### 1. 等待启动状态

OLED 上电初始化完成后显示：

```text
LINE CAR
PRESS KEY
TIME: 00.00s
```

### 2. 按键启动后

显示：

```text
RUNNING
TIME: 08.37s
```

时间至少显示到 0.01 秒：

```text
秒.百分之一秒
```

例如：

```text
TIME: 12.34s
```

时间转换方式：

```c
uint32_t seconds = elapsed_ms / 1000;
uint32_t centiseconds = (elapsed_ms % 1000) / 10;
```

建议格式：

```c
snprintf(buffer,
         sizeof(buffer),
         "TIME:%02lu.%02lus",
         seconds,
         centiseconds);
```

需要避免 `snprintf()` 格式类型不匹配产生编译警告，可根据工程中 `uint32_t` 的实际类型进行安全转换。

### 3. 完成一圈后的处理

先检查工程是否已经有“完成一圈”或“到达 A 点”的停车标志。

如果已经存在：

- 小车停车时停止计时；
- OLED 保持显示最终时间；
- 显示状态改为：

```text
FINISH
TIME: 15.62s
```

如果工程目前没有可靠的完成一圈判断：

- 不要擅自添加未经验证的停车识别算法；
- 只实现启动计时和运行时间显示；
- 在最终说明中指出后续需要接入的停止计时位置。

---

## 八、刷新频率要求

OLED 显示不能影响巡线控制。

禁止在每次主循环中进行完整清屏和全屏刷新。

禁止在定时器中断、GPIO 中断或电机控制中断中刷新 OLED。

OLED 更新时间间隔设置为：

```c
#define OLED_REFRESH_INTERVAL_MS 50
```

或者：

```c
#define OLED_REFRESH_INTERVAL_MS 100
```

主循环参考逻辑：

```c
uint32_t now = Get_Time();

if (timing_running)
{
    elapsed_time_ms = now - start_time_ms;

    if ((uint32_t)(now - last_oled_update_ms) >= OLED_REFRESH_INTERVAL_MS)
    {
        last_oled_update_ms = now;
        OLED_ShowTime(elapsed_time_ms);
    }
}
```

必须使用无符号时间差写法，使系统计时溢出后仍然可以正确判断刷新间隔。

尽量只更新发生变化的显示区域，不要反复执行：

```c
OLED_Clear();
OLED_Update();
```

---

## 九、按键逻辑要求

当前工程已经存在：

```c
Key_Handle();
```

请优先在现有按键事件逻辑中加入计时启动代码，不要在 `main()` 中重新写一套按键扫描。

需要保证：

1. 按键消抖逻辑仍然有效；
2. 一次按下只启动一次；
3. 小车启动和计时启动发生在同一次按键事件中；
4. OLED 计时从按键有效触发的时刻开始；
5. 不改变当前巡线启动逻辑；
6. 不改变按键对应 GPIO；
7. 不影响 `g_LinePortal_flag` 的原有作用。

建议封装接口：

```c
void RaceTimer_Start(void);
void RaceTimer_Stop(void);
void RaceTimer_Reset(void);
uint32_t RaceTimer_GetElapsedMs(void);
bool RaceTimer_IsRunning(void);
```

可新增：

```text
BSP/Timer/race_timer.c
BSP/Timer/race_timer.h
```

但如果现有 Timer 模块已经适合扩展，则直接在现有文件中实现，不要为了形式重复创建模块。

---

## 十、主程序集成要求

现有主循环结构大致为：

```c
while (1)
{
    uint32_t now = Get_Time();

    Key_Handle();

    if (g_LinePortal_flag)
    {
        LineWalking();
    }
}
```

修改时需要保持巡线调用逻辑基本不变。

推荐结构：

```c
while (1)
{
    uint32_t now = Get_Time();

    Key_Handle();

    OLED_Task(now);

    if (g_LinePortal_flag)
    {
        LineWalking();
    }
}
```

建议将显示刷新封装为：

```c
void OLED_Task(uint32_t now_ms);
```

`OLED_Task()` 必须是非阻塞式任务，不能长时间等待。

---

## 十一、初始化顺序

请检查工程当前初始化流程，并按照合理顺序接入：

```c
SYSCFG_DL_init();
Timer_Init();
Key_Init();
OLED_Init();
```

具体初始化函数名应以现有工程为准，不要凭空创建与 SysConfig 重复的初始化代码。

OLED 初始化失败时：

- 不允许阻塞小车运行；
- 可以通过串口输出错误；
- 可以设置 OLED 状态标志；
- 小车巡线功能仍应继续工作。

例如：

```c
static bool oled_ready = false;
```

只有初始化成功后才调用显示刷新。

---

## 十二、代码质量要求

1. 保持现有工程编码风格。
2. 新增函数必须在头文件中声明。
3. 所有新增变量使用明确类型。
4. 避免无意义全局变量。
5. 对共享变量正确使用 `volatile`。
6. 不在中断中调用 OLED、`printf()` 或阻塞式 I²C。
7. 不使用动态内存分配。
8. 不引入 Arduino、HAL 或与 MSPM0 不兼容的库。
9. 使用 TI MSPM0 SDK 现有 DriverLib API。
10. 不删除现有巡线、电机、按键和串口代码。
11. 不修改其他外设引脚。
12. 不大范围重构已经正常工作的巡线程序。
13. 所有 I²C 等待过程必须有超时保护。
14. 编译时不得出现新增的 warning 或 error。

---

## 十三、执行步骤

请按以下顺序执行：

### 第一步：分析工程

先说明：

- 当前启动按键位于哪个文件；
- 按键有效事件在哪里产生；
- `g_LinePortal_flag` 在哪里被置位；
- `Get_Time()` 的单位；
- 当前定时器工作方式；
- PA0、PA1 是否已被其他外设占用；
- 工程中是否已有 I²C 配置；
- 工程中是否已有 OLED 驱动；
- 当前是否有到达 A 点后的停车标志。

分析完成后再修改代码。

### 第二步：配置硬件 I²C

修改 `empty.syscfg`：

```text
I2C0_SDA -> PA0
I2C0_SCL -> PA1
```

只修改 OLED 所需的引脚复用配置，不改变其他引脚。

### 第三步：增加 OLED 驱动

增加 SSD1306 I²C 驱动和基础字符显示功能。

### 第四步：增加计时状态

将按键启动事件和计时启动绑定。

### 第五步：增加非阻塞 OLED 刷新任务

每 50～100ms 更新一次显示时间。

### 第六步：编译验证

执行完整编译，检查：

- 是否有编译错误；
- 是否有链接错误；
- 是否有重复定义；
- SysConfig 是否有引脚冲突；
- OLED 驱动是否已加入编译；
- 是否存在格式化字符串警告；
- 是否影响现有巡线代码。

---

## 十四、验收标准

修改完成后必须满足：

- [ ] OLED 使用硬件 I2C0；
- [ ] PA0 配置为 I2C0_SDA；
- [ ] PA1 配置为 I2C0_SCL；
- [ ] 其他引脚配置保持不变；
- [ ] OLED 上电可以正常初始化；
- [ ] OLED 可以显示英文和数字；
- [ ] 上电后显示等待启动界面；
- [ ] 按键按下时计时从 0 开始；
- [ ] 按键启动小车和启动计时同步；
- [ ] 按住按键不会导致时间反复清零；
- [ ] OLED 实时显示运行时间；
- [ ] 时间至少精确到 0.01 秒；
- [ ] OLED 更新不会明显影响巡线控制；
- [ ] OLED 未连接时程序不会永久卡死；
- [ ] 不在中断中刷新 OLED；
- [ ] 原有巡线功能保持正常；
- [ ] 原有电机、按键、蜂鸣器和 LED 功能保持正常；
- [ ] 工程能够正常编译；
- [ ] 不新增编译警告。

---

## 十五、最终输出要求

完成修改后，请输出：

1. 工程现有逻辑分析；
2. 修改过的文件列表；
3. 每个文件的修改内容；
4. PA0、PA1 的最终 SysConfig 配置；
5. OLED 型号、分辨率和 I²C 地址；
6. 按键启动计时的调用链；
7. OLED 刷新的调用链；
8. 计时开始和停止的位置；
9. 是否发现 PA0、PA1 与 UART0 冲突；
10. 编译结果；
11. 仍需人工验证的内容；
12. OLED 实际接线表。

不要只给出代码片段，请直接在以下工程中完成修改：

```text
E:\my Desktop\2026_Electric_Competition\2026\Competition_PJ
```

修改前先阅读现有代码，尽量复用原有模块，禁止破坏已经可以正常工作的巡线功能。
```