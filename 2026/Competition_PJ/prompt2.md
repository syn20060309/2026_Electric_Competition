# MSPM0G3507 小车由 K1 手动停车改为 A 点自动停车

## 一、任务目标

请直接检查并修改以下最新工程：

```text
E:\my Desktop\2026_Electric_Competition\2026\Competition_PJ
```

当前工程已经实现：

- 八路红外循迹；
- K1 按键启动和停止小车；
- 比赛计时模块；
- SSD1306 OLED 时间显示；
- K1 启动时计时从零开始；
- K1 再次按下时停止小车和计时；
- OLED 能显示运行时间和最终时间。

现在需要将停车逻辑改为：

> K1 只负责启动小车。小车完成一圈并再次经过 A 点横向启停线时，自动停止电机、停止比赛计时，并在 OLED 上保持显示最终时间，不再依赖第二次按下 K1。

请先完整阅读当前最新代码，确认实际函数和变量名称后直接实施，不需要再次提交设计等待确认。

---

## 二、已经确认的实车特征

经过实际观察，八路循迹模块的 LED 表现如下：

- 正常直线行驶时，大多数情况下会亮 6 个灯；
- 极少数情况下会亮 7 个灯；
- 普通赛道上几乎不可能出现 8 个灯同时亮；
- 只有通过 A 点横向启停线的一瞬间，才会出现 8 个灯同时亮。

因此终点识别规则必须是：

```text
6 灯亮：正常循迹，不能停车
7 灯亮：偶发正常情况，不能停车
8 灯同时亮：A 点启停线的核心识别特征
```

禁止使用：

```c
black_count >= 6U
```

禁止使用：

```c
black_count >= 7U
```

作为 A 点判断条件。

A 点核心特征必须是：

```text
八个循迹传感器同时处于“LED 亮/检测到横线”的状态
```

---

## 三、先分析当前工程

修改前必须先检查并说明：

1. 当前 K1 短按和长按事件在哪个文件产生；
2. 当前 K1 如何修改 `g_LinePortal_flag`；
3. 当前停止电机调用了哪些函数；
4. 当前 `RaceTimer_Start()` 和 `RaceTimer_Stop()` 在哪里调用；
5. 当前 OLED 如何区分 `RUNNING` 和 `STOPPED`；
6. `LineWalking()` 在哪里读取八路传感器；
7. 八路传感器读取接口是否能判断通信成功；
8. LED 亮时对应的软件值是 `0` 还是 `1`；
9. 八个传感器变量的排列顺序；
10. 当前停止电机属于 PWM 清零、自由滑行还是已有主动刹车。

分析完成后直接修改代码。

---

## 四、确认八路传感器电平含义

重点检查：

```text
BSP/Eight_Tracking/app_irtracking.c
BSP/Eight_Tracking/app_irtracking.h
```

以及：

```c
deal_IRdata(...)
LineWalking()
x1, x2, x3, x4, x5, x6, x7, x8
```

必须根据现有代码确认：

- LED 亮对应原始值 `0` 还是 `1`；
- 黑色横线对应原始值 `0` 还是 `1`；
- 传感器通信失败时原始变量可能是什么值。

不要直接假设原始数据全为 `1` 就是八灯全亮。

建议将八路原始数据转换成统一的软件掩码：

```text
bit = 1：对应传感器处于 LED 亮/检测到黑色状态
bit = 0：对应传感器未检测到黑色
```

建议实现：

```c
static uint8_t Tracking_BuildActiveMask(
    uint8_t x1,
    uint8_t x2,
    uint8_t x3,
    uint8_t x4,
    uint8_t x5,
    uint8_t x6,
    uint8_t x7,
    uint8_t x8);
```

经过统一转换后：

```c
active_mask == 0xFFU
```

才表示八个灯同时亮。

---

## 五、A 点自动识别条件

正常终点识别必须同时满足：

```text
小车已经离开起点横线
并且比赛计时已经达到 14 秒
并且本次八路传感器读取有效
并且八个灯同时亮
```

推荐集中定义：

```c
#define FINISH_MIN_TIME_MS              14000U
#define START_LINE_CLEAR_CONFIRM_MS       100U
#define FINISH_ALL_ACTIVE_MASK           0xFFU
```

终点判断逻辑应类似：

```c
bool finish_detection_enabled =
    start_line_cleared &&
    RaceTimer_GetElapsedMs() >= FINISH_MIN_TIME_MS;

if (finish_detection_enabled &&
    sensor_data_valid &&
    active_mask == FINISH_ALL_ACTIVE_MASK)
{
    /* 锁存 A 点终点事件 */
}
```

其中 14 秒只负责解锁 A 点检测，不能单独触发停车。

严禁：

```c
if (RaceTimer_GetElapsedMs() >= FINISH_MIN_TIME_MS)
{
    Car_FinishStop();
}
```

正常停车必须由 14 秒之后出现的有效八灯全亮触发。

---

## 六、起点防误判

小车启动时可能正压在 A 点启停线上，此时可能已经八灯全亮。

因此 K1 启动后不能立即允许终点识别。

启动后先进入：

```text
LAP_STATE_LEAVING_START
```

只有八灯全亮状态消失，并连续保持至少：

```c
#define START_LINE_CLEAR_CONFIRM_MS 100U
```

才认为小车已经离开起点：

```c
start_line_cleared = true;
```

参考逻辑：

```c
if (active_mask != FINISH_ALL_ACTIVE_MASK)
{
    if (!clear_candidate_active)
    {
        clear_candidate_active = true;
        clear_candidate_start_ms = now_ms;
    }
    else if ((uint32_t)(now_ms - clear_candidate_start_ms)
             >= START_LINE_CLEAR_CONFIRM_MS)
    {
        start_line_cleared = true;
    }
}
else
{
    clear_candidate_active = false;
}
```

时间达到 14 秒以后，只有 `start_line_cleared == true` 才允许识别终点。

---

## 七、八灯全亮采用瞬时有效采样

根据实车观察，八灯全亮只会在经过 A 点横线时瞬间出现。

因此不要要求：

```text
八灯全亮连续保持 20ms
```

不要要求：

```text
八灯全亮连续出现多次
```

否则可能错过很短的全亮时刻。

推荐规则：

```c
if (finish_detection_enabled &&
    sensor_data_valid &&
    active_mask == 0xFFU)
{
    finish_line_detected = true;
}
```

只要出现一次有效八灯全亮，就立即锁存：

```c
finish_line_detected = true;
```

锁存后即使下一次采样恢复成 6 灯或 7 灯，也不能取消终点事件。

必须保证同一轮测试只触发一次。

---

## 八、传感器数据有效性

由于终点识别只需要一次八灯全亮采样，因此必须防止 I²C 读取异常被误认为终点。

请检查当前 `deal_IRdata()` 是否能够报告读取成功。

推荐接口：

```c
bool deal_IRdata(
    uint8_t *x1,
    uint8_t *x2,
    uint8_t *x3,
    uint8_t *x4,
    uint8_t *x5,
    uint8_t *x6,
    uint8_t *x7,
    uint8_t *x8);
```

成功读取返回：

```c
true
```

I²C 超时、NACK 或数据异常返回：

```c
false
```

如果不适合修改现有函数签名，可以增加：

```c
bool EightTracking_LastReadValid(void);
```

终点判断必须包含：

```c
sensor_data_valid == true
```

读取失败时：

- 不得更新自动停车状态；
- 不得把旧数据或默认值当成八灯全亮；
- 不得触发停车；
- 保持当前巡线模块已有的错误处理方式。

修改传感器读取接口时，尽量保持原有巡线算法行为不变。

---

## 九、独立自动停车状态机

建议新增：

```text
BSP/Eight_Tracking/lap_finish.c
BSP/Eight_Tracking/lap_finish.h
```

如果当前工程有更合适的模块位置，也可以放入现有结构，但不要把全部逻辑堆进 `empty.c`。

建议状态：

```c
typedef enum
{
    LAP_STATE_IDLE = 0,
    LAP_STATE_LEAVING_START,
    LAP_STATE_RUNNING,
    LAP_STATE_FINISH_DETECTED,
    LAP_STATE_FINISHED,
    LAP_STATE_ABORTED,
    LAP_STATE_TIMEOUT
} LapFinish_State;
```

### LAP_STATE_IDLE

等待 K1 启动。

### LAP_STATE_LEAVING_START

K1 已经启动小车和比赛计时，正在等待确认离开起点横线。

### LAP_STATE_RUNNING

已经离开起点，正常巡线。

该状态中：

- 比赛时间未达到 14 秒：忽略八灯全亮终点判断；
- 比赛时间达到 14 秒：允许识别一次有效八灯全亮。

### LAP_STATE_FINISH_DETECTED

已经检测并锁存 A 点横线事件，准备执行最终停车。

第一版应立即进入最终停车，不增加阻塞延时。

### LAP_STATE_FINISHED

电机停止，比赛计时停止，OLED 显示最终时间。

### LAP_STATE_ABORTED

人工紧急停止，不能显示成正常完成。

### LAP_STATE_TIMEOUT

超时安全停止，不能显示成正常完成。

---

## 十、建议状态机接口

建议提供：

```c
void LapFinish_Init(void);
void LapFinish_Reset(void);
void LapFinish_Start(uint32_t now_ms);

bool LapFinish_Update(
    uint32_t now_ms,
    bool sensor_data_valid,
    uint8_t active_mask);

LapFinish_State LapFinish_GetState(void);

void LapFinish_MarkFinished(void);
void LapFinish_MarkAborted(void);
void LapFinish_MarkTimeout(void);

bool LapFinish_IsFinished(void);
bool LapFinish_IsAborted(void);
```

`LapFinish_Update()` 返回 `true` 时，表示已经检测到 A 点，应当执行自动停车。

接口可以根据现有工程风格调整，但必须满足：

- 不在状态机内刷新 OLED；
- 不在状态机内执行阻塞延时；
- 不在状态机内重复读取八路传感器；
- 不破坏现有巡线算法。

---

## 十一、必须复用同一次八路采样

当前 `LineWalking()` 已经读取一次八路传感器。

自动停车判断必须复用这一帧数据。

推荐结构：

```c
void LineWalking(void)
{
    uint8_t x1, x2, x3, x4;
    uint8_t x5, x6, x7, x8;

    bool sensor_valid = deal_IRdata(
        &x1, &x2, &x3, &x4,
        &x5, &x6, &x7, &x8);

    uint8_t active_mask = Tracking_BuildActiveMask(
        x1, x2, x3, x4,
        x5, x6, x7, x8);

    if (LapFinish_Update(
            Get_Time(),
            sensor_valid,
            active_mask))
    {
        Car_FinishStop();
        return;
    }

    /* 保持原有循迹算法不变 */
}
```

禁止在以下位置再次读取八路传感器：

```c
LapFinish_Update()
```

禁止在主循环中额外读取一遍八路传感器。

自动停车检测和循迹算法必须使用同一帧数据。

---

## 十二、检测后立即锁存并停止

检测到一次有效八灯全亮后：

```c
finish_line_detected = true;
```

随后立即执行自动停车流程：

```c
Car_FinishStop();
```

第一版不要增加：

- 阻塞式延时；
- 反向制动；
- 临时反转电机；
- 未验证的复杂刹车算法。

应优先调用当前工程已经验证过的停止接口。

如果当前停止方式只是 PWM 清零或驱动关闭，小车可能因为惯性继续滑行少量距离。这属于后续实车停车精度调节问题，本次任务先保证自动识别和可靠停止。

---

## 十三、统一正常停车函数

建议封装：

```c
void Car_FinishStop(void);
```

正常自动完成时执行：

```c
void Car_FinishStop(void)
{
    /* 先禁止主循环继续执行循迹 */
    g_LinePortal_flag = 0;

    /* 调用工程现有的可靠停车接口 */
    Motor_Stop();

    /* 在实际发出停车指令时停止比赛计时 */
    RaceTimer_Stop();

    /* 锁存正常完成状态 */
    LapFinish_MarkFinished();
}
```

如果工程中没有 `Motor_Stop()`，必须使用当前已经存在的左右电机停止函数。

不要凭空调用不存在的函数。

停车时必须确保：

- 左轮停止；
- 右轮停止；
- 后续不再执行 `LineWalking()`；
- 后续代码不会重新写入非零电机速度；
- 比赛计时停止；
- 最终时间保持；
- 自动完成状态只设置一次；
- 下一次 K1 可以重新开始一轮测试。

执行顺序可以根据现有代码调整，但不能出现停车后 `LineWalking()` 又覆盖电机指令的问题。

---

## 十四、修改 K1 行为

当前 K1 逻辑可能是：

```text
第一次短按：启动
第二次短按：停车
```

现在改为：

### 小车处于 IDLE、FINISHED、ABORTED 或 TIMEOUT

K1 短按：

```c
LapFinish_Reset();
RaceTimer_Start();
LapFinish_Start(Get_Time());
g_LinePortal_flag = 1;
```

小车从零开始新一轮测试。

必须保证：

- 使用现有按键消抖事件；
- 一次按下只启动一次；
- 小车启动和比赛计时同步；
- OLED 切换到 `RUNNING`；
- 自动停车状态被正确复位；
- 上一轮最终时间不会影响新一轮。

### 小车处于运行状态

运行中的第二次短按不再作为正常停车方式。

正式逻辑中建议直接忽略运行中的短按。

可以使用宏保留临时调试功能：

```c
#define ENABLE_K1_MANUAL_STOP_DEBUG 0
```

正式比赛时必须为：

```c
#define ENABLE_K1_MANUAL_STOP_DEBUG 0
```

### K1 长按紧急停车

为了安全，建议保留 K1 长按紧急停止：

```c
#define ENABLE_K1_EMERGENCY_STOP 1
```

长按时：

```c
g_LinePortal_flag = 0;
Motor_Stop();
RaceTimer_Stop();
LapFinish_MarkAborted();
```

紧急停止状态必须是：

```text
ABORT
```

不能标记为：

```text
FINISH
```

如果当前 K1 模块无法可靠区分短按和长按，不要为此大范围重写按键驱动。可以先保留当前已有的长按机制，或者通过宏控制调试停止行为。

---

## 十五、OLED 状态修改

OLED 功能已经实现，保持当前硬件 I²C 和刷新机制不变。

不得修改：

```text
PA0 → I2C0_SDA
PA1 → I2C0_SCL
```

### 等待启动

```text
LINE CAR
PRESS KEY
TIME:00.00s
```

### 正常运行

```text
RUNNING
TIME:xx.xx s
```

### A 点自动完成

```text
FINISH
TIME:xx.xx s
```

### K1 长按紧急停止

```text
ABORT
TIME:xx.xx s
```

### 安全超时停止

```text
TIMEOUT
TIME:xx.xx s
```

OLED 仍然由现有：

```c
OLED_Task(now_ms);
```

在主循环中更新。

禁止在以下位置直接执行 OLED I²C 刷新：

- `LapFinish_Update()`；
- `Car_FinishStop()`；
- `deal_IRdata()`；
- `LineWalking()` 的终点分支；
- 定时器中断；
- GPIO 中断；
- 电机中断。

`OLED_Task()` 根据：

```c
LapFinish_GetState()
```

以及：

```c
RaceTimer_GetElapsedMs()
```

决定显示内容。

---

## 十六、安全超时

建议增加异常安全超时：

```c
#define LAP_SAFETY_TIMEOUT_MS 35000U
```

如果比赛计时超过 35 秒仍未检测到 A 点：

```c
g_LinePortal_flag = 0;
Motor_Stop();
RaceTimer_Stop();
LapFinish_MarkTimeout();
```

OLED 显示：

```text
TIMEOUT
TIME:xx.xx s
```

安全超时不能显示：

```text
FINISH
```

超时判断只是防止小车无限运行，不是正常终点识别方式。

---

## 十七、集中配置参数

将自动停车相关参数集中放置：

```c
#define FINISH_MIN_TIME_MS                14000U
#define START_LINE_CLEAR_CONFIRM_MS         100U
#define FINISH_ALL_ACTIVE_MASK             0xFFU

#define LAP_SAFETY_TIMEOUT_MS             35000U

#define ENABLE_K1_MANUAL_STOP_DEBUG            0
#define ENABLE_K1_EMERGENCY_STOP               1
#define LAP_FINISH_DEBUG                       1
```

正式比赛前设置：

```c
#define LAP_FINISH_DEBUG 0
```

不要把这些参数分散到多个 `.c` 文件中。

---

## 十八、调试输出

增加可关闭的串口调试信息：

```c
#define LAP_FINISH_DEBUG 1
```

调试内容至少包括：

```text
lap state
elapsed time
sensor valid
active mask
active count
start line cleared
finish detection enabled
all-eight-active event
finish trigger
timeout trigger
```

示例：

```text
LAP state=RUNNING time=13920 mask=0x3F valid=1 enabled=0
LAP state=RUNNING time=14310 mask=0x7F valid=1 enabled=1
LAP state=RUNNING time=16842 mask=0xFF valid=1 enabled=1
LAP A-LINE DETECTED
LAP FINISHED time=16843
```

普通状态信息限制为每 100ms 或更慢输出一次。

八灯全亮、自动停车和超时事件可以立即输出一次。

正式比赛前关闭调试，避免串口输出影响巡线实时性。

---

## 十九、推荐主循环

保持主循环总体结构不变：

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

不要在主循环中额外读取八路传感器。

自动停车判断应放在 `LineWalking()` 已有传感器读取之后。

---

## 二十、推荐完整流程

```text
系统上电
    ↓
OLED 显示等待启动
    ↓
K1 短按有效事件
    ↓
重置自动停车状态
    ↓
RaceTimer_Start()
    ↓
启动循迹
    ↓
等待确认离开起点横线
    ↓
正常巡线
    ↓
比赛时间达到 14 秒
    ↓
允许识别 A 点
    ↓
收到一次有效的八灯全亮采样
    ↓
立即锁存 A 点事件
    ↓
清除 g_LinePortal_flag
    ↓
调用现有电机停止接口
    ↓
RaceTimer_Stop()
    ↓
状态变为 FINISHED
    ↓
OLED 显示 FINISH 和最终时间
```

正常完成不再需要第二次按下 K1。

---

## 二十一、必须保持不变

不得修改：

- OLED 的 PA0、PA1 配置；
- OLED I2C0 驱动；
- 八路传感器的 I2C 引脚；
- UART 引脚；
- 电机引脚；
- K1 物理引脚；
- 蜂鸣器引脚；
- LED 引脚；
- 已经正常工作的 OLED 底层驱动；
- 已经正常工作的比赛计时；
- 原有巡线算法和转向参数；
- 原有电机速度参数；
- 其他与自动停车无关的外设。

除非实现自动停车确实需要，否则不要大范围重构已经正常工作的代码。

---

## 二十二、验收标准

完成后必须满足：

- [ ] K1 短按可以启动小车；
- [ ] K1 启动时比赛计时从零开始；
- [ ] K1 启动后 OLED 显示 `RUNNING`；
- [ ] 运行中的第二次短按不再作为正常停车；
- [ ] 启动时八灯全亮不会立即停车；
- [ ] 小车能够确认已经离开起点；
- [ ] 时间不足 14 秒时不能识别正常终点；
- [ ] 14 秒本身不能直接触发正常停车；
- [ ] 6 灯亮不能触发停车；
- [ ] 7 灯亮不能触发停车；
- [ ] 一次有效的 8 灯全亮可以触发 A 点；
- [ ] 不要求 8 灯全亮持续多次；
- [ ] 不要求 8 灯全亮持续固定时间；
- [ ] I²C 读取失败不能触发停车；
- [ ] A 点事件触发后立即锁存；
- [ ] 自动停车检测复用 `LineWalking()` 的同一帧数据；
- [ ] 不重复读取八路 I²C 传感器；
- [ ] A 点检测后自动停止左右电机；
- [ ] 自动停车后不再调用 `LineWalking()`；
- [ ] 自动停车时停止 `RaceTimer`；
- [ ] OLED 自动显示 `FINISH` 和最终时间；
- [ ] 正常完成不再依赖第二次按 K1；
- [ ] 下一次 K1 可以重新开始新一轮；
- [ ] K1 长按可以作为紧急停止；
- [ ] 紧急停止显示 `ABORT`；
- [ ] 安全超时显示 `TIMEOUT`；
- [ ] 不使用阻塞延时完成自动停车；
- [ ] 不擅自增加反向制动；
- [ ] 原有巡线和 OLED 功能保持正常；
- [ ] SysConfig 无新增引脚冲突；
- [ ] CCS 全量编译无新增 error 和 warning。

---

## 二十三、执行步骤

请直接执行：

1. 阅读当前最新工程；
2. 找到 K1 当前启动和停车调用链；
3. 找到比赛计时启动和停止位置；
4. 找到 OLED 当前状态显示逻辑；
5. 找到 `LineWalking()` 的八路读取位置；
6. 确认 LED 亮对应的软件电平；
7. 确认传感器读取有效性；
8. 新增或扩展自动停车状态机；
9. 将 K1 短按改为只启动；
10. 保留或完善 K1 长按紧急停止；
11. 增加离开起点确认；
12. 增加 14 秒终点检测解锁；
13. 使用一次有效八灯全亮锁存 A 点；
14. 调用现有可靠电机停止接口；
15. 自动停止比赛计时；
16. 增加 `FINISH / ABORT / TIMEOUT` OLED 状态；
17. 运行 SysConfig 检查；
18. 执行 CCS Clean Build / Rebuild；
19. 修复所有新增 error 和 warning。

现在直接修改代码，不需要再等待确认。

---

## 二十四、最终输出要求

完成后输出：

1. 当前 K1 原有行为；
2. 修改后的 K1 行为；
3. 实际修改文件列表；
4. LED 亮对应的软件值；
5. 八灯全亮对应的原始数据；
6. 统一转换后的 `active_mask`；
7. A 点识别完整条件；
8. 14 秒解锁条件所在位置；
9. 起点防误判方法；
10. 传感器数据有效性判断方法；
11. 自动停车状态机；
12. 实际调用的电机停止函数；
13. 当前停止方式是自由滑行还是已有主动刹车；
14. RaceTimer 自动停止位置；
15. OLED 的 `FINISH / ABORT / TIMEOUT` 显示逻辑；
16. SysConfig 检查结果；
17. CCS 全量编译结果；
18. 仍需实车验证的内容。

不得修改 OLED 的 PA0/PA1 配置，不得改变其他硬件引脚，不得破坏已经正常工作的巡线、计时和 OLED 功能。
