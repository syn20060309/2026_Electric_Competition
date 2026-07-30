# Acceleration OLED Scheduler Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Periodically sample MPU6050 acceleration, show X/Y acceleration and race time together on the OLED, and schedule key, acceleration, OLED, and odometry tasks without reducing line-tracking call frequency.

**Architecture:** A checked MPU6050 register-read API feeds a focused acceleration task that owns the latest sample. A small cooperative scheduler invokes key handling at 10ms, acceleration sampling at 30ms, OLED refresh at 100ms, and odometry at 1000ms; `LineWalking()` remains directly in the main loop. The OLED consumes the cached sample and never initiates MPU6050 I2C traffic.

**Tech Stack:** MSPM0G3507 DriverLib, software I2C, SSD1306 OLED over hardware I2C1, C11, TI Arm Clang, GCC host tests.

## Global Constraints

- OLED acceleration uses signed `g` values with three decimal places.
- No periodic UART acceleration output is added.
- One six-byte MPU6050 transaction reads X, Y, and Z together.
- Key handling runs every 10ms, acceleration every 30ms, OLED every 100ms, and odometry every 1000ms.
- `LineWalking()` remains outside the scheduler and runs on every eligible main-loop iteration.
- Existing tracking speed, PID, lap finish, I2C assignments, and generated SysConfig files remain unchanged.
- Preserve the user's current uncommitted `Get_Odometry()` implementation in `BSP/Motor/app_motor.c`.

---

### Task 1: Checked acceleration sampling

**Files:**
- Modify: `BSP/MPU6050/bsp_mpu6050.h`
- Modify: `BSP/MPU6050/bsp_mpu6050.c`
- Create: `BSP/MPU6050/acceleration_task.h`
- Create: `BSP/MPU6050/acceleration_task.c`
- Create: `tests/test_mpu6050_acceleration.c`
- Create: `tests/stubs/bsp_mpu6050_accel.h`

**Interfaces:**
- Consumes: `char MPU6050_ReadData(uint8_t, uint8_t, uint8_t, uint8_t *)`.
- Produces: `bool MPU6050ReadAccChecked(short *acc_data)`, `void MPU6050_AccelTask(void)`, and `bool MPU6050_AccelGetLatest(MPU6050_AccelSample *sample)`.

- [ ] **Step 1: Write the failing acceleration-task test**

Test a successful raw sample `{16384, -8192, 0}` and assert that the cached
sample is valid with `x_g == 1.0f` and `y_g == -0.5f`. Then make the fake
driver fail and assert that the latest sample becomes invalid.

```c
static bool next_read_ok;
static short next_raw[3];

bool MPU6050ReadAccChecked(short *data)
{
    if (!next_read_ok) {
        return false;
    }
    data[0] = next_raw[0];
    data[1] = next_raw[1];
    data[2] = next_raw[2];
    return true;
}
```

- [ ] **Step 2: Run the test to verify RED**

```powershell
gcc -std=c11 -Wall -Wextra -Werror `
  -Itests/stubs -IBSP/MPU6050 `
  tests/test_mpu6050_acceleration.c `
  BSP/MPU6050/acceleration_task.c `
  -o .tmp/test_mpu6050_acceleration.exe
```

Expected: compilation fails because `acceleration_task.c/.h` do not exist.

- [ ] **Step 3: Implement the checked driver and cached task**

Add a driver function that reads registers beginning at
`MPU6050_ACC_OUT`, returns `false` on I2C failure, and only writes the
caller's three values after all six bytes were received successfully.
Keep `MPU6050ReadAcc()` and implement it by calling the checked API.

Define the cached sample:

```c
typedef struct {
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;
    float x_g;
    float y_g;
    float z_g;
    bool valid;
} MPU6050_AccelSample;
```

`MPU6050_AccelTask()` performs one checked read. On success it divides each
raw value by `16384.0f`; on failure it sets `valid` to false.
`MPU6050_AccelGetLatest()` copies the snapshot and returns its validity.

- [ ] **Step 4: Run the acceleration test to verify GREEN**

Compile with the Step 2 command and run:

```powershell
.tmp/test_mpu6050_acceleration.exe
```

Expected: exit code 0.

### Task 2: Cooperative scheduler and odometry declarations

**Files:**
- Create: `BSP/task.h`
- Create: `BSP/task.c`
- Modify: `BSP/Motor/app_motor.h`
- Modify: `BSP/Motor/app_motor.c`
- Create: `tests/test_scheduler.c`
- Create: `tests/stubs/key.h`
- Create: `tests/stubs/acceleration_task.h`
- Create: `tests/stubs/oled_task.h`
- Create: `tests/stubs/app_motor.h`
- Create: `tests/stubs/timer.h`

**Interfaces:**
- Consumes: `Get_Time`, `Key_Handle`, `MPU6050_AccelTask`,
  `OLED_RefreshTask`, and `Get_Odometry`.
- Produces: `void Scheduler_Init(void)` and `void Scheduler_Run(void)`.

- [ ] **Step 1: Write the failing scheduler test**

Use fake task functions that increment counters. After `Scheduler_Init()`:

- time 9ms: no calls;
- time 10ms: one key call;
- time 30ms: acceleration called once;
- time 100ms: OLED called once;
- time 1000ms: odometry called once;
- reset near `UINT32_MAX` and cross zero: unsigned subtraction still triggers
  each task at the correct interval.

- [ ] **Step 2: Run the test to verify RED**

```powershell
gcc -std=c11 -Wall -Wextra -Werror `
  -Itests/stubs -IBSP `
  tests/test_scheduler.c BSP/task.c `
  -o .tmp/test_scheduler.exe
```

Expected: compilation fails because `task.c/.h` do not exist.

- [ ] **Step 3: Implement the scheduler**

Define:

```c
typedef struct {
    uint32_t interval;
    uint32_t last_call;
    void (*task)(void);
} Task;
```

Use this task table:

```c
static Task tasks[] = {
    {10U,   0U, Key_Handle},
    {30U,   0U, MPU6050_AccelTask},
    {100U,  0U, OLED_RefreshTask},
    {1000U, 0U, Get_Odometry},
};
```

`Scheduler_Init()` sets every `last_call` to the current `Get_Time()`.
`Scheduler_Run()` reads time once, checks `(uint32_t)(now - last_call)`,
executes each due task once, and assigns `last_call = now`.

In `app_motor.c`, preserve the user's function and add:

```c
uint8_t encoder_odometry_flag = 1U;
float odometry_sum = 0.0f;
```

Declare both variables and `void Get_Odometry(void)` in `app_motor.h`.

- [ ] **Step 4: Run the scheduler test to verify GREEN**

Compile with the Step 2 command and run:

```powershell
.tmp/test_scheduler.exe
```

Expected: exit code 0.

### Task 3: Four-line OLED page

**Files:**
- Modify: `BSP/OLED/oled_task.h`
- Modify: `BSP/OLED/oled_task.c`
- Modify: `tests/test_oled_task.c`

**Interfaces:**
- Consumes: `MPU6050_AccelGetLatest`, `RaceTimer_GetState`,
  `RaceTimer_GetElapsedMs`, `LapFinish_GetState`, and `Get_Time`.
- Produces: `void OLED_RefreshTask(void)` and the four-line OLED page.

- [ ] **Step 1: Extend the OLED test and verify RED**

Stub `MPU6050_AccelGetLatest()` and assert:

```text
RUNNING
AX:+0.008g
AY:-0.003g
T:12.34s
```

Also assert that invalid acceleration renders `AX:---` and `AY:---`, idle
uses `PRESS KEY` on row 6, and finished/aborted/timeout states retain their
existing labels and frozen time.

Compile:

```powershell
gcc -std=c11 -Wall -Wextra -Werror `
  -IBSP/OLED -IBSP/MPU6050 -IBSP/Eight_Tracking -IBSP/Timer `
  tests/test_oled_task.c BSP/OLED/oled_task.c `
  BSP/Eight_Tracking/lap_finish.c `
  BSP/Eight_Tracking/tracking_sample.c BSP/Timer/race_timer.c `
  -o .tmp/test_oled_task.exe
```

Expected: assertions fail because acceleration rows and row-6 time are not
implemented.

- [ ] **Step 2: Implement the page without floating-point printf**

Move the time row to row 6. Format acceleration by rounding `g * 1000` to a
signed milli-g integer and print sign, integer part, and three fractional
digits using integer `snprintf`, for example `AX:+0.008g`.
`OLED_RefreshTask()` calls `OLED_Task(Get_Time())`.

- [ ] **Step 3: Run the OLED test to verify GREEN**

Compile with the Step 1 command and run:

```powershell
.tmp/test_oled_task.exe
```

Expected: exit code 0.

### Task 4: Main-loop integration and full verification

**Files:**
- Modify: `empty.c`
- Verify: `empty.syscfg`
- Verify: all production and test sources

**Interfaces:**
- Consumes: `Scheduler_Init` and `Scheduler_Run`.
- Produces: scheduled support tasks while preserving direct line tracking.

- [ ] **Step 1: Integrate the scheduler**

After timer startup and existing task initializations, call
`Scheduler_Init()`. Replace direct main-loop calls to `Key_Handle()` and
`OLED_Task(now)` with `Scheduler_Run()`. Keep:

```c
if (g_LinePortal_flag)
{
    LineWalking();
}
```

directly in the loop after the scheduler.

- [ ] **Step 2: Run all host tests**

Compile every test with `-std=c11 -Wall -Wextra -Werror` and execute every
result. Expected: all existing and new tests exit 0.

- [ ] **Step 3: Run SysConfig generation**

```powershell
E:\ti\sysconfig_1.24.1\sysconfig_cli.bat `
  --product E:\ti\mspm0_sdk_2_05_01_00\.metadata\product.json `
  --script empty.syscfg --output .tmp\syscfg-accel-scheduler `
  --compiler ticlang
```

Expected: zero errors; PA24/PA25 MPU6050 GPIO, I2C0 tracking pins, and I2C1
OLED pins remain unchanged.

- [ ] **Step 4: Run a forced TI/CCS full build**

Use the existing `.tmp/ccs_full_build.mk` harness with `gmake -B`, adding
the new production sources if the harness has an explicit source list.
Expected: link succeeds with no new TI compiler errors or warnings.

- [ ] **Step 5: Review the final diff**

Run `git diff --check`, verify no generated SysConfig source was edited,
confirm `printf_i2c_data()` remains without a periodic call, and confirm
tracking speed/PID constants are unchanged.

