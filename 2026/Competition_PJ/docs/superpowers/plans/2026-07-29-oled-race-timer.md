# OLED Race Timer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a fault-tolerant SSD1306 OLED on I2C0 and synchronize its elapsed-time display with the existing debounced K1 car start/stop event.

**Architecture:** A three-state race timer reuses the existing 1 ms `Get_Time()` clock. A bounded-polling SSD1306 driver maintains a framebuffer and dirty ranges, while a 100 ms UI task renders IDLE, RUNNING, and STOPPED screens outside interrupts.

**Tech Stack:** TI MSPM0 DriverLib/SysConfig, C11/TI Arm Clang, native GCC host tests, CCS managed build.

## Global Constraints

- Modify `E:\my Desktop\2026_Electric_Competition\2026\Competition_PJ` directly.
- OLED is SSD1306-compatible, 128×64, hardware I2C0, unshifted 7-bit address `0x3C`, 100 kHz.
- PA0 is I2C0 SDA and PA1 is I2C0 SCL; every other pin assignment remains unchanged.
- K1 first valid event starts the car and resets timing; the next valid event stops both and freezes the time.
- Do not add an A-point or lap-finish algorithm.
- Every I2C wait has a timeout; OLED failure cannot block line following.
- Do not hand-edit generated `ti_msp_dl_config.c/.h`.

---

### Task 1: Three-state race timer

**Files:**
- Create: `tests/test_race_timer.c`
- Create: `BSP/Timer/race_timer.h`
- Create: `BSP/Timer/race_timer.c`

**Interfaces:**
- Consumes: `uint32_t Get_Time(void)`
- Produces: `RaceTimer_Start`, `RaceTimer_Stop`, `RaceTimer_Reset`, `RaceTimer_GetElapsedMs`, `RaceTimer_IsRunning`, and `RaceTimer_GetState`

- [ ] **Step 1: Write the failing host test**

Test literal outcomes for IDLE at power-on, RUNNING elapsed time, STOPPED frozen
time, restart from zero, stop-while-idle, and `UINT32_MAX` wraparound using a
fake `Get_Time()`.

- [ ] **Step 2: Run the test to verify RED**

Run:

```powershell
gcc -std=c11 -Wall -Wextra -Werror tests/test_race_timer.c BSP/Timer/race_timer.c -IBSP/Timer -o tests/test_race_timer.exe
```

Expected: compilation fails because the production race-timer files/API do not
exist.

- [ ] **Step 3: Implement the minimal state machine**

Use:

```c
typedef enum {
    RACE_TIMER_IDLE,
    RACE_TIMER_RUNNING,
    RACE_TIMER_STOPPED
} RaceTimerState;
```

Store start tick, frozen elapsed time, and state. Use unsigned subtraction for
elapsed time.

- [ ] **Step 4: Verify GREEN**

Compile with the command above and run `tests/test_race_timer.exe`; expect exit
code 0 and all assertions to pass.

### Task 2: OLED UI behavior

**Files:**
- Create: `tests/test_oled_task.c`
- Create: `BSP/OLED/oled_task.h`
- Create: `BSP/OLED/oled_task.c`
- Create: `BSP/OLED/oled.h` initially as the UI-facing driver contract

**Interfaces:**
- Consumes: race-timer state and OLED drawing functions
- Produces: `OLED_TaskInit(void)` and `OLED_Task(uint32_t now_ms)`

- [ ] **Step 1: Write the failing UI test**

Link `oled_task.c` against fake OLED functions and the real race timer. Record
rendered strings and verify:

- successful initialization renders `LINE CAR`, `PRESS KEY`, and
  `TIME:00.00s`;
- failed initialization causes later tasks to make no writes;
- RUNNING renders `RUNNING` and a hand-checked time such as `TIME:08.37s`;
- STOPPED renders `STOPPED`, freezes the string, and does not become IDLE;
- calls before 100 ms do not refresh.

- [ ] **Step 2: Run the test to verify RED**

Run:

```powershell
gcc -std=c11 -Wall -Wextra -Werror tests/test_oled_task.c BSP/OLED/oled_task.c BSP/Timer/race_timer.c -IBSP/OLED -IBSP/Timer -o tests/test_oled_task.exe
```

Expected: compilation fails because the task implementation does not exist.

- [ ] **Step 3: Implement the minimal UI task**

Initialize once, return immediately if `OLED_IsReady()` is false, compare
`RaceTimer_GetState()` with the previous state, format seconds and
centiseconds using `snprintf`, and refresh with unsigned
`now_ms - last_update_ms >= 100U`.

- [ ] **Step 4: Verify GREEN**

Compile and run the UI test; expect exit code 0 with all assertions passing.

### Task 3: SSD1306 driver and hardware I2C0

**Files:**
- Modify: `empty.syscfg`
- Create: `BSP/OLED/oled.c`
- Complete: `BSP/OLED/oled.h`
- Create: `BSP/OLED/oled_font.c`
- Create: `BSP/OLED/oled_font.h`
- Modify: `.cproject`

**Interfaces:**
- Consumes: generated `OLED_INST` DriverLib definitions
- Produces: initialization, readiness, framebuffer drawing, command/data
  writes, and dirty-range update functions declared in `oled.h`

- [ ] **Step 1: Add the SysConfig I2C controller**

Create an `OLED` I2C instance configured as I2C0 controller, Standard 100 kHz,
PA0 SDA, and PA1 SCL. Leave the existing Sensor I2C1 instance and all other
assignments byte-for-byte unchanged.

- [ ] **Step 2: Generate SysConfig outputs in a temporary build directory**

Run SysConfig CLI with the installed SDK product metadata and
`--treatWarningsAsErrors`. Confirm the generated header contains `OLED_INST`
and the command exits 0.

- [ ] **Step 3: Implement the failing-safe SSD1306 driver**

Use a 1024-byte framebuffer, 6×8 printable ASCII font, dirty page column
ranges, and the SSD1306 initialization sequence. All controller idle/busy
polling uses a `Get_Time()` deadline plus an iteration guard. On transfer
error/timeout, reset and flush the controller and mark the OLED unavailable.

- [ ] **Step 4: Add the OLED include directory**

Append `${PROJECT_ROOT}/BSP/OLED` to the compiler include search paths without
changing existing paths.

### Task 4: Existing K1 and main-loop integration

**Files:**
- Modify: `BSP/Key/key.c`
- Modify: `empty.c`

**Interfaces:**
- Consumes: race timer and OLED task
- Produces: synchronized car/timer start-stop and main-loop display refresh

- [ ] **Step 1: Bind the existing valid event**

After the current short/long event toggles `g_LinePortal_flag`, call
`RaceTimer_Start()` on true and `RaceTimer_Stop()` on false. Preserve the
existing motor stop call and debounce state machine.

- [ ] **Step 2: Integrate initialization and task**

After TIMG0 starts, call `OLED_TaskInit()`. In the existing loop call
`OLED_Task(now)` after `Key_Handle()` and before the unchanged
`if (g_LinePortal_flag) LineWalking();`.

- [ ] **Step 3: Re-run host tests**

Compile and run both host tests with `-Wall -Wextra -Werror`; expect both to
pass.

### Task 5: Full generation, build, and acceptance

**Files:**
- Modify only files required to fix discovered build errors or warnings
- Update: `README.md` with OLED wiring and display behavior

**Interfaces:**
- Consumes: complete CCS project
- Produces: generated SysConfig outputs and a warning-free firmware image

- [ ] **Step 1: Run final SysConfig validation**

Generate with warnings treated as errors and inspect final assignments for
OLED I2C0 PA0/PA1 and Sensor I2C1 PA16/PA15.

- [ ] **Step 2: Run CCS full clean build**

Use the installed CCS command-line managed-build tooling with the MSPM0 SDK
and TI Arm Clang product variables. Require exit code 0 and no newly introduced
warnings or errors.

- [ ] **Step 3: Verify the acceptance checklist**

Review the final diff, confirm no generated source was hand-edited, confirm
existing peripheral and line-following code is unchanged outside the two
planned integration points, and list hardware-only checks.

- [ ] **Step 4: Remove host test executables**

Delete only `tests/test_race_timer.exe` and `tests/test_oled_task.exe`; retain
the test sources.
