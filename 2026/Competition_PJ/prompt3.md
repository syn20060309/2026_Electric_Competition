

# 调整 A 点自动停车判定：6/7/8 路同时有效均可停车

## 一、工程位置

请直接检查并修改当前最新工程：

```text
E:\my Desktop\2026_Electric_Competition\2026\Competition_PJ
```

当前工程已经完成：

- 八路红外循迹；
- K1 启动小车；
- 比赛计时；
- OLED 时间显示；
- A 点自动停车状态机；
- 14 秒后允许识别终点；
- 原判定条件为八路传感器全部有效才停车。

现在只调整 A 点识别条件，其他功能保持不变。

请直接修改代码，不需要再次提交设计等待确认。

---

## 二、最新实车观察

调整八路红外传感器安装位置后，实车表现如下：

- 正常直线循迹时，一般只有中间 4 个或 5 个灯亮；
- 正常巡线过程中很难出现 6 个灯同时亮；
- 小车通过 A 点横向启停线时，由于车身可能存在轻微倾斜，可能出现：
  - 6 个灯同时亮；
  - 7 个灯同时亮；
  - 8 个灯同时亮；
- 原来只允许 8 个灯同时亮才停车，存在经过 A 点但未停车的概率。

因此将 A 点候选特征放宽为：

```text
6、7、8 个传感器同时有效，均认为检测到 A 点启停线
```

也就是：

```c
active_count >= 6U
```

---

## 三、保持时间门槛不变

当前终点检测的最小时间门槛保持为：

```c
#define FINISH_MIN_TIME_MS 14000U
```

14 秒只负责解锁终点检测，不能单独触发停车。

严禁改成：

```c
if (elapsed_ms >= FINISH_MIN_TIME_MS)
{
    Car_FinishStop();
}
```

正常停车仍必须同时检测到 A 点宽黑线特征。

最终基本条件为：

```c
start_line_cleared &&
elapsed_ms >= FINISH_MIN_TIME_MS &&
sensor_data_valid &&
active_count >= FINISH_ACTIVE_COUNT_THRESHOLD
```

---

## 四、将固定全亮掩码改为数量阈值

原代码可能使用：

```c
active_mask == FINISH_ALL_ACTIVE_MASK
```

或者：

```c
active_mask == 0xFFU
```

作为终点条件。

现在将其改为可调数量阈值：

```c
#define FINISH_ACTIVE_COUNT_THRESHOLD 6U
```

终点候选判断改为：

```c
uint8_t active_count = Tracking_CountActive(active_mask);

bool finish_pattern_detected =
    sensor_data_valid &&
    active_count >= FINISH_ACTIVE_COUNT_THRESHOLD;
```

因此：

```text
active_count = 4：不停车
active_count = 5：不停车
active_count = 6：允许触发
active_count = 7：允许触发
active_count = 8：允许触发
```

不得使用：

```c
active_count == 6U
```

因为这样会排除 7 路和 8 路。

必须使用：

```c
active_count >= 6U
```

---

## 五、复用或新增统一计数函数

如果当前工程已经存在类似函数：

```c
Tracking_CountActive(active_mask)
```

请复用，不要重复实现。

如果该函数当前只在：

```c
#if LAP_FINISH_DEBUG
```

内部定义，则将其移动到可供正式终点判定使用的位置。

建议接口：

```c
uint8_t Tracking_CountActive(uint8_t active_mask);
```

参考实现：

```c
uint8_t Tracking_CountActive(uint8_t active_mask)
{
    uint8_t count = 0U;

    while (active_mask != 0U)
    {
        count += (uint8_t)(active_mask & 1U);
        active_mask >>= 1U;
    }

    return count;
}
```

不要因为关闭 `LAP_FINISH_DEBUG` 而导致正式逻辑无法使用该函数。

---

## 六、修改自动停车状态机

重点检查：

```text
BSP/Eight_Tracking/lap_finish.c
BSP/Eight_Tracking/lap_finish.h
```

以及当前实际存放 A 点识别逻辑的文件。

将原有的：

```c
active_mask == 0xFFU
```

改为：

```c
active_count >= FINISH_ACTIVE_COUNT_THRESHOLD
```

推荐结构：

```c
LapFinish_Event LapFinish_Update(
    uint32_t now_ms,
    uint32_t elapsed_ms,
    bool sensor_valid,
    uint8_t active_mask)
{
    uint8_t active_count;

    if (!sensor_valid)
    {
        return LAP_FINISH_EVENT_NONE;
    }

    active_count = Tracking_CountActive(active_mask);

    /* 保留现有离开起点状态处理。 */

    if (!LapFinish_StartLineCleared())
    {
        return LAP_FINISH_EVENT_NONE;
    }

    if (elapsed_ms < FINISH_MIN_TIME_MS)
    {
        return LAP_FINISH_EVENT_NONE;
    }

    if (active_count >= FINISH_ACTIVE_COUNT_THRESHOLD)
    {
        /* 锁存 A 点终点事件。 */
        return LAP_FINISH_EVENT_FINISH;
    }

    return LAP_FINISH_EVENT_NONE;
}
```

具体结构应适配当前已有状态机，不要删除当前：

- `IDLE`；
- `LEAVING_START`；
- `RUNNING`；
- `FINISHED`；
- `ABORTED`；
- `TIMEOUT`

等已有状态。

---

## 七、更新离开起点的判断

原来的起点离开判断可能使用：

```c
active_mask != 0xFFU
```

由于现在 A 点特征已经改为：

```c
active_count >= 6U
```

起点清除条件也应采用同一套阈值语义。

在 `LAP_STATE_LEAVING_START` 中：

```c
bool start_line_present =
    sensor_valid &&
    active_count >= FINISH_ACTIVE_COUNT_THRESHOLD;
```

只有检测到：

```c
active_count < FINISH_ACTIVE_COUNT_THRESHOLD
```

并连续保持当前已有的：

```c
#define START_LINE_CLEAR_CONFIRM_MS 100U
```

才认为小车已经离开起点：

```c
start_line_cleared = true;
```

推荐逻辑：

```c
if (sensor_valid &&
    active_count < FINISH_ACTIVE_COUNT_THRESHOLD)
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
        state = LAP_STATE_RUNNING;
    }
}
else
{
    clear_candidate_active = false;
}
```

不要继续把“不是八灯全亮”直接等价为“已经离开起点”。

---

## 八、建议增加可调的采样确认次数

由于终点阈值从 8 路放宽为 6 路，为降低偶发单帧噪声误停车的概率，建议增加一个集中配置参数：

```c
#define FINISH_CONFIRM_SAMPLE_COUNT 2U
```

表示在时间门槛已经满足后，需要连续两次有效采样均满足：

```c
active_count >= 6U
```

才确认 A 点。

参考逻辑：

```c
if (sensor_valid &&
    active_count >= FINISH_ACTIVE_COUNT_THRESHOLD)
{
    if (finish_confirm_count < FINISH_CONFIRM_SAMPLE_COUNT)
    {
        finish_confirm_count++;
    }

    if (finish_confirm_count >= FINISH_CONFIRM_SAMPLE_COUNT)
    {
        finish_confirm_count = 0U;
        return LAP_FINISH_EVENT_FINISH;
    }
}
else
{
    finish_confirm_count = 0U;
}
```

本次默认：

```c
#define FINISH_CONFIRM_SAMPLE_COUNT 2U
```

原因：

- 正常巡线一般只有 4～5 路有效；
- A 点横线会达到 6～8 路；
- 连续两次确认可以过滤单次 I²C 数据抖动；
- 不采用毫秒阻塞延时；
- 不要求 6 路以上持续很长时间。

如果实车测试发现横线经过太快而漏检，只需要把：

```c
#define FINISH_CONFIRM_SAMPLE_COUNT 2U
```

改为：

```c
#define FINISH_CONFIRM_SAMPLE_COUNT 1U
```

不得为此改动状态机整体结构。

---

## 九、终点触发后立即锁存

一旦满足：

```c
start_line_cleared &&
elapsed_ms >= FINISH_MIN_TIME_MS &&
sensor_valid &&
active_count >= 6U
```

并达到配置的确认采样次数，就立即锁存正常完成事件。

随后调用现有：

```c
Car_FinishStop();
```

必须保持：

```c
g_LinePortal_flag = 0;
Control_Pwm(0, 0, 0, 0);
RaceTimer_Stop();
LapFinish_MarkFinished();
```

或当前工程中等价的可靠停车流程。

不要修改当前电机引脚、PWM 引脚和电机控制参数。

不要增加反向制动或阻塞式延时。

---

## 十、调试输出调整

当前调试信息中应显示：

```text
active mask
active count
终点阈值
时间是否解锁
起点是否已离开
连续确认次数
最终触发事件
```

例如：

```text
LAP state=RUNNING time=15320 valid=1 mask=0x3C count=4 threshold=6 enabled=1 confirm=0
LAP state=RUNNING time=16420 valid=1 mask=0x7E count=6 threshold=6 enabled=1 confirm=1
LAP state=RUNNING time=16423 valid=1 mask=0xFE count=7 threshold=6 enabled=1 confirm=2
LAP FINISH TRIGGER time=16423
```

调试输出仍必须限频，不能每次巡线循环都执行长格式 `printf()`。

正式比赛前仍设置：

```c
#define LAP_FINISH_DEBUG 0
```

---

## 十一、OLED 和计时逻辑保持不变

不得修改 OLED 的硬件配置：

```text
PA0 → I2C0_SDA
PA1 → I2C0_SCL
```

不得修改传感器 I2C1、UART、电机、K1、蜂鸣器和 LED 的任何硬件引脚。

保持：

```c
#define FINISH_MIN_TIME_MS 14000U
```

保持现有显示：

```text
运行中：
RUNNING
TIME:xx.xx s

自动完成：
FINISH
TIME:xx.xx s
```

不要修改 RaceTimer 的计时精度和刷新周期。

---

## 十二、不得改动的内容

本次任务只调整 A 点自动停车识别阈值。

不得改动：

- OLED 的 PA0、PA1；
- 传感器硬件引脚；
- UART 引脚；
- 电机引脚；
- K1 引脚；
- 蜂鸣器和 LED 引脚；
- SysConfig 外设分配；
- 当前巡线 PID 参数；
- 当前巡线速度；
- 当前转向逻辑；
- 当前 OLED 驱动；
- 当前 RaceTimer 实现；
- 当前 I²C 超时处理；
- 当前电机停车函数。

`empty.syscfg` 不需要修改。

---

## 十三、更新测试

请同步更新或新增自动测试，至少覆盖：

```text
时间不足 14 秒 + 8 路有效：不能完成
时间达到 14 秒 + 5 路有效：不能完成
时间达到 14 秒 + 6 路有效：可以完成
时间达到 14 秒 + 7 路有效：可以完成
时间达到 14 秒 + 8 路有效：可以完成
传感器读取无效 + 8 路数据：不能完成
尚未离开起点 + 8 路有效：不能完成
连续确认次数不足：不能完成
连续确认次数达到配置值：触发完成
```

如果默认：

```c
#define FINISH_CONFIRM_SAMPLE_COUNT 2U
```

测试必须验证：

```text
第一次 count=6：不立即完成
第二次连续 count=6/7/8：触发完成
中间掉回 count=5：确认计数清零
```

---

## 十四、验收标准

修改完成后必须满足：

- [ ] 14 秒时间门槛保持不变；
- [ ] 14 秒不能单独触发停车；
- [ ] 5 路及以下有效不能触发停车；
- [ ] 6 路有效可以触发 A 点候选；
- [ ] 7 路有效可以触发 A 点候选；
- [ ] 8 路有效可以触发 A 点候选；
- [ ] 终点阈值集中定义为宏；
- [ ] 起点离开判断同步使用 6 路阈值；
- [ ] 启动位置不会被立即误判为终点；
- [ ] 传感器读取无效时不能停车；
- [ ] A 点事件触发后只锁存一次；
- [ ] 自动停止电机；
- [ ] 自动停止 RaceTimer；
- [ ] OLED 显示 `FINISH` 和最终时间；
- [ ] K1 启动逻辑保持不变；
- [ ] 巡线算法和 PID 参数保持不变；
- [ ] 所有硬件引脚保持不变；
- [ ] `empty.syscfg` 不修改；
- [ ] CCS 全量编译无新增 error 和 warning；
- [ ] 所有相关测试通过。

---

## 十五、执行要求

请直接执行以下工作：

1. 阅读当前最新 `lap_finish.c/.h`；
2. 找到当前 `active_mask == 0xFFU` 的终点判断；
3. 将判断改为 `active_count >= 6U`；
4. 将阈值定义为 `FINISH_ACTIVE_COUNT_THRESHOLD`；
5. 确保计数函数在关闭调试后仍可使用；
6. 同步修改起点离开判断；
7. 增加可调连续采样确认次数，默认 2 次；
8. 保持 14 秒时间门槛；
9. 保持硬件引脚和 SysConfig 不变；
10. 更新测试；
11. 执行完整编译和测试；
12. 修复所有新增 warning 和 error。

现在直接修改代码，不需要再次等待确认。

---

## 十六、最终输出

完成后请说明：

1. 实际修改文件；
2. 原终点判定条件；
3. 新终点判定条件；
4. `FINISH_ACTIVE_COUNT_THRESHOLD` 的值；
5. `FINISH_CONFIRM_SAMPLE_COUNT` 的值；
6. 起点离开判断如何同步调整；
7. 6、7、8 路有效的测试结果；
8. 传感器无效数据的测试结果；
9. 14 秒时间门槛是否保持不变；
10. SysConfig 是否保持未修改；
11. CCS 全量编译结果；
12. 实车仍需观察的情况。
```

我的建议是先使用：

```c
#define FINISH_ACTIVE_COUNT_THRESHOLD 6U
#define FINISH_CONFIRM_SAMPLE_COUNT   2U
```

它比“单帧出现 6 路就停车”稳一些，同时比原来的“必须 8 路全亮”更不容易漏检。