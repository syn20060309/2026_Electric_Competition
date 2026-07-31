# MSPM0G3507 小车增加双速度模式与 K1 状态切换

## 一、工程位置

请直接检查并修改当前最新工程：

```text
E:\my Desktop\2026_Electric_Competition\2026\Competition_PJ
```

当前工程已经实现：

- 八路红外循迹；
- K1 短按启动；
- K1 长按识别；
- A 点自动停车；
- RaceTimer 比赛计时；
- SSD1306 OLED 状态和时间显示；
- 蜂鸣器；
- 第二问巡线速度约为 335；
- 当前自动停车、巡线和 OLED 功能已经能够正常运行。

现在需要增加第二种运行模式，使同一套程序可以分别完成：

1. 第二问高速巡线模式；
2. 第 4、5、6 问稳球低速模式。

请先完整阅读当前最新代码，确认现有按键、蜂鸣器、OLED、巡线速度和自动停车调用链后再修改。

直接进入实施，不需要再次提交设计等待确认。

---

## 二、两种速度模式

集中定义：

```c
#define SPEED_QUESTION_2       335
#define SPEED_BALL_CONTROL     230
```

含义：

### 第二问高速模式

```text
模式名称：Q2 FAST
巡线速度：335
用途：第二问，一圈时间必须不超过 20 秒
```

### 稳球低速模式

```text
模式名称：BALL CONTROL
巡线速度：230
用途：第 4、5、6 问，优先降低小车振动并提高钢球稳定性
```

暂时只切换巡线基础速度。

不得自动修改：

- 巡线 PID 参数；
- 转向输出参数；
- 自动停车条件；
- 14 秒终点检测门槛；
- OLED I²C 配置；
- 硬件引脚。

后续如果实车测试证明两个速度需要不同 PID，再单独增加模式参数。

---

## 三、模式状态和运行状态必须分开

不要将“速度模式”和“小车运行状态”混在同一个变量中。

新增模式枚举：

```c
typedef enum
{
    CAR_MODE_NONE = 0,
    CAR_MODE_QUESTION_2,
    CAR_MODE_BALL_CONTROL
} CarMode;
```

现有运行状态可以继续复用。

如果当前没有统一运行状态，可使用或适配：

```c
typedef enum
{
    CAR_STATE_IDLE = 0,
    CAR_STATE_RUNNING,
    CAR_STATE_FINISHED,
    CAR_STATE_ABORTED,
    CAR_STATE_TIMEOUT
} CarState;
```

其中：

- `CarMode` 决定使用哪个巡线速度；
- `CarState` 决定小车当前是否正在运行；
- 模式切换只允许在小车停止时进行；
- 运行过程中禁止切换速度模式。

---

## 四、上电初始化行为

系统完成全部初始化后：

1. 小车保持静止；
2. 当前模式设置为：

```c
CAR_MODE_NONE
```

3. 蜂鸣器快速短响两声；
4. OLED 显示等待选择模式。

蜂鸣节奏：

```c
#define BUZZER_INIT_ON_MS       60U
#define BUZZER_INIT_GAP_MS      60U
```

声音效果：

```text
滴-滴
```

OLED 建议显示：

```text
SELECT MODE
HOLD K1
```

或者根据现有 128×64 排版显示：

```text
SELECT MODE
LONG: CHANGE
SHORT: START
```

在 `CAR_MODE_NONE` 状态下，短按 K1 不允许启动小车。

---

## 五、停止状态下长按 K1：切换模式

当小车处于以下停止状态时：

```text
IDLE
FINISHED
ABORTED
TIMEOUT
```

长按 K1 用于切换速度模式。

切换顺序：

```text
CAR_MODE_NONE
    ↓ 长按
CAR_MODE_QUESTION_2
    ↓ 长按
CAR_MODE_BALL_CONTROL
    ↓ 长按
CAR_MODE_QUESTION_2
```

第一次长按选择第二问模式。

此后每次长按在两个模式之间循环。

不要从 `BALL_CONTROL` 切换回 `NONE`。

---

## 六、第二问模式提示

进入：

```c
CAR_MODE_QUESTION_2
```

时：

1. 蜂鸣器响一声；
2. OLED 显示模式名称和速度；
3. 小车继续保持停止。

蜂鸣参数：

```c
#define BUZZER_Q2_ON_MS         120U
```

声音效果：

```text
滴
```

OLED 显示建议：

```text
MODE: Q2 FAST
SPEED: 335
PRESS TO START
```

如果屏幕空间不足，可以简化为：

```text
Q2 FAST
SPD:335
PRESS K1
```

---

## 七、稳球模式提示

进入：

```c
CAR_MODE_BALL_CONTROL
```

时：

1. 蜂鸣器响两声；
2. 两声之间的间隔明显大于初始化提示；
3. OLED 显示稳球模式和速度；
4. 小车继续保持停止。

蜂鸣参数：

```c
#define BUZZER_BALL_ON_MS       120U
#define BUZZER_BALL_GAP_MS      250U
```

声音效果：

```text
滴——滴
```

需要明显区别于初始化完成时的：

```text
滴-滴
```

OLED 显示建议：

```text
MODE: BALL
SPEED: 230
PRESS TO START
```

或者：

```text
BALL CTRL
SPD:230
PRESS K1
```

---

## 八、停止状态下短按 K1：启动当前模式

当已经选择有效模式时，短按 K1 启动小车。

允许启动的模式：

```c
CAR_MODE_QUESTION_2
CAR_MODE_BALL_CONTROL
```

启动流程继续复用现有代码：

```text
K1 短按有效事件
→ 重置 LapFinish
→ RaceTimer 从零开始
→ 设置小车为运行状态
→ 设置 g_LinePortal_flag
→ OLED 显示 RUNNING
→ 按当前模式速度巡线
```

不要复制第二套启动流程。

应封装或复用统一启动接口，例如：

```c
void Car_StartSelectedMode(void);
```

参考逻辑：

```c
void Car_StartSelectedMode(void)
{
    if (CarMode_GetCurrent() == CAR_MODE_NONE)
    {
        return;
    }

    LapFinish_Reset();
    RaceTimer_Start();
    LapFinish_Start(Get_Time());

    g_LinePortal_flag = 1;
}
```

具体接口名称以当前工程为准。

---

## 九、未选择模式时短按 K1

如果当前模式为：

```c
CAR_MODE_NONE
```

短按 K1 时：

- 不启动电机；
- 不启动 RaceTimer；
- 不改变 `g_LinePortal_flag`；
- OLED 继续提示选择模式。

可以增加一个短错误提示音，例如：

```text
较短的一声
```

但不要让它与第二问模式的一声提示完全相同。

如果不希望增加新的蜂鸣节奏，可以直接忽略短按并保持 OLED 提示。

---

## 十、运行状态下的按键行为

当小车正在运行时：

### 短按 K1

```text
忽略
```

不得：

- 重新开始计时；
- 切换模式；
- 修改速度；
- 作为正常停车操作。

正常停车继续由 A 点自动停车状态机触发。

### 长按 K1

作为紧急停车：

```text
RUNNING + 长按 K1
→ 停止电机
→ 停止 RaceTimer
→ 标记 ABORTED
→ OLED 显示 ABORT
```

紧急停车不得标记为正常：

```text
FINISH
```

运行中的长按只负责紧急停止，不能切换模式。

---

## 十一、防止长按同时触发短按

必须检查当前 K1 状态机。

一次长按操作只能产生一个：

```c
KEY_EVENT_LONG
```

按键释放时不得继续产生：

```c
KEY_EVENT_SHORT
```

正确行为：

```c
if (press_duration_ms >= KEY_LONG_PRESS_MS)
{
    event = KEY_EVENT_LONG;
}
else
{
    event = KEY_EVENT_SHORT;
}
```

建议长按时间：

```c
#define KEY_LONG_PRESS_MS       800U
```

如果当前工程已经定义长按时间，继续复用现有值，不要无必要修改。

需要使用长按锁存标志，例如：

```c
long_press_reported = true;
```

长按事件触发后，必须等待 K1 完全释放，才允许产生下一次按键事件。

否则按住 K1 不放可能导致：

```text
Q2 → BALL → Q2 → BALL
```

不断切换。

---

## 十二、模式管理模块

建议新增独立模块：

```text
BSP/Mode/car_mode.c
BSP/Mode/car_mode.h
```

如果当前已有合适的控制模块，例如：

```text
race_control.c/.h
```

也可以在现有模块中扩展，但不要将全部状态逻辑直接堆入 `empty.c`。

建议接口：

```c
void CarMode_Init(void);
void CarMode_SelectNext(void);

CarMode CarMode_GetCurrent(void);
uint16_t CarMode_GetSpeed(void);
bool CarMode_IsSelected(void);

const char *CarMode_GetDisplayName(void);
```

参考速度接口：

```c
uint16_t CarMode_GetSpeed(void)
{
    switch (g_car_mode)
    {
        case CAR_MODE_QUESTION_2:
            return SPEED_QUESTION_2;

        case CAR_MODE_BALL_CONTROL:
            return SPEED_BALL_CONTROL;

        case CAR_MODE_NONE:
        default:
            return 0U;
    }
}
```

---

## 十三、修改巡线基础速度

当前 `app_irtracking.c` 中可能存在固定速度：

```c
#define IRR_SPEED 350
```

以及：

```c
Motion_Car_Control(IRR_SPEED, 0, pid_output_IRR);
```

改为从模式模块读取当前速度：

```c
uint16_t current_speed = CarMode_GetSpeed();

Motion_Car_Control(
    current_speed,
    0,
    pid_output_IRR);
```

如果模式无效或返回速度为 0：

```c
if (current_speed == 0U)
{
    Car_Stop();
    return;
}
```

不要在巡线函数内部根据按键切换模式。

巡线函数只读取当前已选速度。

删除或停止使用固定的：

```c
IRR_SPEED
```

但不要修改现有 PID 参数和转向算法。

---

## 十四、完成一圈后的行为

A 点自动停车后：

```text
停止电机
→ RaceTimer_Stop()
→ LapFinish 状态变为 FINISHED
→ OLED 显示 FINISH 和最终时间
```

当前选择的速度模式必须保留。

例如本轮使用：

```text
BALL CONTROL
```

完成后仍保持：

```c
CAR_MODE_BALL_CONTROL
```

这样重复测试同一个项目时，只需要再次短按 K1，无需重新选择模式。

如果需要切换到另一模式：

```text
停车状态下长按 K1
```

---

## 十五、OLED 显示逻辑

保持现有 OLED 硬件和刷新机制不变：

```text
PA0 → I2C0_SDA
PA1 → I2C0_SCL
```

不得修改 OLED 硬件引脚或 SysConfig。

### 初始化后未选模式

```text
SELECT MODE
HOLD K1
```

### 已选第二问模式，尚未启动

```text
Q2 FAST
SPD:350
PRESS K1
```

### 已选稳球模式，尚未启动

```text
BALL CTRL
SPD:230
PRESS K1
```

### 运行中

建议显示当前模式：

第二问：

```text
RUN Q2
TIME:18.20s
```

稳球模式：

```text
RUN BALL
TIME:27.80s
```

### 正常完成

```text
FINISH Q2
TIME:18.20s
```

或：

```text
FINISH BALL
TIME:27.80s
```

### 紧急停车

```text
ABORT
TIME:xx.xx s
```

OLED 仍由当前：

```c
OLED_Task(now_ms);
```

在主循环中更新。

禁止在按键中断、定时器中断或模式切换函数内直接刷新 OLED。

模式切换函数只更新状态，由 `OLED_Task()` 在下一次刷新时更新屏幕。

---

## 十六、蜂鸣器实现

建议封装：

```c
void Buzzer_NotifyInitComplete(void);
void Buzzer_NotifyQ2Mode(void);
void Buzzer_NotifyBallMode(void);
```

对应节奏：

```c
#define BUZZER_INIT_ON_MS       60U
#define BUZZER_INIT_GAP_MS      60U

#define BUZZER_Q2_ON_MS        120U

#define BUZZER_BALL_ON_MS      120U
#define BUZZER_BALL_GAP_MS     250U
```

### 初始化提示

```text
响 60ms
停 60ms
响 60ms
```

### 第二问模式

```text
响 120ms
```

### 稳球模式

```text
响 120ms
停 250ms
响 120ms
```

优先使用非阻塞蜂鸣任务。

如果当前蜂鸣器驱动只有阻塞式接口，初始化和停止状态下短暂阻塞可以接受，但：

- 不得在小车运行过程中使用阻塞式模式提示；
- 运行中长按紧急停车，应先立即停止电机，再进行声音提示；
- 不得让蜂鸣器延时影响巡线控制。

---

## 十七、推荐状态流程

```text
系统上电
    ↓
完成初始化
    ↓
快速短响两声
    ↓
CAR_MODE_NONE
    ↓
OLED：SELECT MODE

停止状态长按 K1
    ↓
CAR_MODE_QUESTION_2
    ↓
蜂鸣器响一声
    ↓
OLED：Q2 FAST / SPD 350

短按 K1
    ↓
使用速度 350 启动
    ↓
自动停车
    ↓
保留 Q2 模式

停止状态长按 K1
    ↓
CAR_MODE_BALL_CONTROL
    ↓
蜂鸣器慢间隔响两声
    ↓
OLED：BALL CTRL / SPD 230

短按 K1
    ↓
使用速度 230 启动
    ↓
自动停车
    ↓
保留 BALL 模式
```

---

## 十八、硬件和现有功能约束

本次不得修改：

- OLED PA0、PA1；
- 传感器 I2C 引脚；
- UART 引脚；
- 电机引脚；
- K1 引脚；
- 蜂鸣器引脚；
- LED 引脚；
- SysConfig 外设分配；
- A 点自动停车条件；
- 14 秒终点检测时间门槛；
- RaceTimer 精度；
- OLED I²C 驱动；
- 当前巡线 PID 参数；
- 当前转向算法。

只增加：

- 双速度模式；
- 停止状态下的模式切换；
- 模式蜂鸣提示；
- OLED 模式显示；
- 巡线速度根据模式动态选择。

---

## 十九、测试要求

至少验证以下情况：

### 初始化

- 上电完成后模式为 `NONE`；
- 小车保持停止；
- 初始化快速响两声；
- OLED 显示 `SELECT MODE`。

### 模式选择

- 第一次长按选择 Q2；
- Q2 只响一声；
- 第二次长按选择 BALL；
- BALL 慢间隔响两声；
- 再次长按回到 Q2；
- 长按一次只切换一次。

### 启动

- 未选择模式时短按不能启动；
- Q2 模式短按后速度为 350；
- BALL 模式短按后速度为 230；
- 启动时 RaceTimer 从零开始；
- 启动时自动停车状态正确重置。

### 运行中按键

- 运行中短按被忽略；
- 运行中长按执行紧急停车；
- 运行中长按不切换模式；
- 长按释放时不会额外触发短按。

### 完成后

- 自动停车逻辑保持正常；
- OLED 显示最终时间；
- 当前模式继续保留；
- 再次短按可以重复当前模式；
- 长按可以切换到另一模式。

---

## 二十、验收标准

- [ ] 增加 `CAR_MODE_QUESTION_2`；
- [ ] 增加 `CAR_MODE_BALL_CONTROL`；
- [ ] 上电默认 `CAR_MODE_NONE`；
- [ ] 初始化完成快速短响两声；
- [ ] 长按选择 Q2 时响一声；
- [ ] 长按选择 BALL 时慢间隔响两声；
- [ ] 初始化两声与 BALL 两声节奏明显不同；
- [ ] 停止状态允许切换模式；
- [ ] 运行状态禁止切换模式；
- [ ] 未选择模式不能启动；
- [ ] Q2 模式使用速度 350；
- [ ] BALL 模式使用速度 230；
- [ ] 短按启动当前已选模式；
- [ ] 长按不会同时触发短按；
- [ ] 运行中短按被忽略；
- [ ] 运行中长按可以紧急停车；
- [ ] 自动完成后保留当前模式；
- [ ] A 点自动停车保持正常；
- [ ] RaceTimer 保持正常；
- [ ] OLED 显示当前模式；
- [ ] 所有硬件引脚保持不变；
- [ ] `empty.syscfg` 不修改；
- [ ] CCS 全量编译无新增 error 和 warning。

---

## 二十一、执行步骤

请直接执行：

1. 阅读当前 K1 状态机；
2. 阅读当前蜂鸣器驱动；
3. 阅读当前 OLED Task；
4. 阅读当前巡线速度使用位置；
5. 新增或扩展模式管理模块；
6. 添加 Q2 和 BALL 两个模式；
7. 实现停止状态长按切换模式；
8. 防止长按同时触发短按；
9. 实现三种不同蜂鸣提示；
10. 将固定巡线速度改为当前模式速度；
11. 修改 OLED 模式显示；
12. 保持自动停车和计时功能不变；
13. 更新或新增测试；
14. 执行 CCS Clean Build / Rebuild；
15. 修复所有新增 error 和 warning。

现在直接修改代码，不需要再次等待确认。

---

## 二十二、最终输出要求

完成后请说明：

1. 实际修改文件列表；
2. 新增的模式枚举；
3. Q2 和 BALL 模式速度；
4. K1 短按、长按在各状态下的行为；
5. 如何避免长按后产生短按事件；
6. 初始化蜂鸣节奏；
7. Q2 模式蜂鸣节奏；
8. BALL 模式蜂鸣节奏；
9. OLED 各状态显示内容；
10. 自动完成后是否保留当前模式；
11. 运行中长按紧急停车逻辑；
12. 是否修改 SysConfig；
13. CCS 全量编译结果；
14. 仍需实车测试的项目。
