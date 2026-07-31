# Dual-Speed Car Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add selectable Q2 FAST and BALL CONTROL speed modes, with stopped-state K1 selection, non-blocking buzzer feedback, mode-aware OLED screens, and guarded race startup.

**Architecture:** A new pure `BSP/Mode/car_mode` module owns only the selected mode and its base speed. Key event dispatch coordinates mode selection, buzzer notification, race start, and emergency stop; race/lap state remains owned by the existing timer and lap modules. OLED and line tracking consume the selected mode without mutating it.

**Tech Stack:** C11, TI MSPM0 DriverLib, SysConfig-generated hardware layer, GCC host regression tests, TI Arm Clang full build.

## Global Constraints

- `SPEED_QUESTION_2` is 335 and `SPEED_BALL_CONTROL` is 230.
- Do not modify PID constants, steering rules, finish detection, RaceTimer precision, hardware pins, OLED I2C driver, or `empty.syscfg`.
- Mode starts as `CAR_MODE_NONE`; automatic finish, abort, and timeout preserve the selected mode.
- A stopped long press selects the next mode; a running long press aborts without changing mode.
- A running short press is ignored; a stopped short press starts only a selected mode.
- Buzzer patterns are non-blocking and advance from the existing 1 ms timer interrupt.

---

### Task 1: Pure car mode module

**Files:**
- Create: `BSP/Mode/car_mode.h`
- Create: `BSP/Mode/car_mode.c`
- Create: `tests/test_car_mode.c`

**Interfaces:**
- Produces: `CarMode_Init`, `CarMode_SelectNext`, `CarMode_GetCurrent`, `CarMode_GetSpeed`, `CarMode_IsSelected`, `CarMode_GetDisplayName`, `CarMode_GetShortName`.

- [ ] **Step 1: Write the failing test**

Test that initialization yields NONE/0, then selections cycle `NONE -> Q2 -> BALL -> Q2`, with speeds 335 and 230 and stable display names.

- [ ] **Step 2: Run test to verify it fails**

Run:
`gcc -std=c11 -Wall -Wextra -Werror -IBSP/Mode tests/test_car_mode.c BSP/Mode/car_mode.c -o .tmp/test_car_mode.exe`

Expected: FAIL because the module does not exist.

- [ ] **Step 3: Implement the module**

Define:

```c
typedef enum {
    CAR_MODE_NONE = 0,
    CAR_MODE_QUESTION_2,
    CAR_MODE_BALL_CONTROL
} CarMode;

#define SPEED_QUESTION_2   335U
#define SPEED_BALL_CONTROL 230U
```

Keep all state private to `car_mode.c`.

- [ ] **Step 4: Run test and verify it passes**

Use the command from Step 2 and execute `.tmp/test_car_mode.exe`.

### Task 2: Guarded start and state-aware key dispatch

**Files:**
- Modify: `BSP/Eight_Tracking/race_control.h`
- Modify: `BSP/Eight_Tracking/race_control.c`
- Modify: `BSP/Key/key.h`
- Modify: `BSP/Key/key.c`
- Modify: `tests/test_race_control.c`
- Create: `tests/test_key_mode_control.c`

**Interfaces:**
- Consumes: `CarMode_IsSelected`, `CarMode_SelectNext`, buzzer notification APIs.
- Produces: `bool Car_StartSelectedMode(uint32_t now_ms)` and `void Key_ProcessEvent(KeyEvent event, uint32_t now_ms)`.

- [ ] **Step 1: Write failing race-control and key tests**

Assert that NONE cannot start, selected modes can start, running short presses do nothing, stopped long presses select/notify once, and running long presses abort without changing mode. Drive `Key_Scan` through press/hold/release to prove one long event and no release short event.

- [ ] **Step 2: Run tests and verify failure**

Compile the tests with GCC and expect missing interfaces or incorrect current behavior.

- [ ] **Step 3: Implement minimal dispatch**

`Key_ProcessEvent` uses this decision table:

```text
SHORT + stopped + selected -> Car_StartSelectedMode
SHORT + running            -> ignore
SHORT + NONE               -> ignore
LONG  + running            -> Car_AbortStop
LONG  + stopped            -> CarMode_SelectNext + matching buzzer notice
```

Keep the existing `KEY_STATE_LONG` release behavior so no short event follows a long event.

- [ ] **Step 4: Run both tests and verify pass**

Compile with `-Wall -Wextra -Werror` and execute both binaries.

### Task 3: Non-blocking buzzer patterns

**Files:**
- Modify: `BSP/Buzzer/buzzer.h`
- Modify: `BSP/Buzzer/buzzer.c`
- Modify: `tests/stubs/ti_msp_dl_config.h`
- Create: `tests/test_buzzer_pattern.c`

**Interfaces:**
- Produces: `Buzzer_NotifyInitComplete`, `Buzzer_NotifyQ2Mode`, `Buzzer_NotifyBallMode`.

- [ ] **Step 1: Write failing timing tests**

Assert these 1 ms sequences through `Buzzer_Handle`:

```text
INIT: ON 60, OFF 60, ON 60
Q2:   ON 120
BALL: ON 120, OFF 250, ON 120
```

- [ ] **Step 2: Run and verify failure**

Expected: missing notification functions.

- [ ] **Step 3: Implement a small ISR-driven pattern state machine**

Starting a pattern turns the buzzer on immediately. Each call to `Buzzer_Handle` consumes 1 ms, transitions between ON/GAP phases, and leaves the buzzer off after the last phase. Do not call `delay_ms` from notification APIs.

- [ ] **Step 4: Run timing tests and verify pass**

Compile and execute with host stubs.

### Task 4: Dynamic tracking speed

**Files:**
- Modify: `BSP/Eight_Tracking/app_irtracking.c`

**Interfaces:**
- Consumes: `uint16_t CarMode_GetSpeed(void)`.

- [ ] **Step 1: Remove fixed `IRR_SPEED` consumption**

Read `CarMode_GetSpeed()` once per `LineWalking` call after valid sensor/finish processing. If it returns zero, issue `Contrl_Pwm(0,0,0,0)` and return; otherwise pass it unchanged to `Motion_Car_Control`.

- [ ] **Step 2: Compile the full project**

Verify no PID or steering expression changed.

### Task 5: Mode-aware OLED rendering

**Files:**
- Modify: `BSP/OLED/oled_task.c`
- Modify: `tests/test_oled_task.c`

**Interfaces:**
- Consumes: current `CarMode`, speed, display name, and short name.

- [ ] **Step 1: Write failing OLED tests**

Cover NONE selection prompt, selected Q2/BALL idle screens, `RUN Q2`/`RUN BALL`, `FINISH Q2`/`FINISH BALL`, and mode preservation after stop. Keep elapsed time and acceleration rendering covered.

- [ ] **Step 2: Run and verify failure**

Expected: current generic IDLE/RUNNING/FINISH labels do not match.

- [ ] **Step 3: Implement mode-aware labels**

Cache the displayed mode alongside timer/lap state so a selection change refreshes on the next task call. Do not perform OLED writes from key or mode code.

- [ ] **Step 4: Run and verify pass**

Compile OLED test with car mode, race timer, lap finish, and tracking sample modules.

### Task 6: Initialization integration and complete verification

**Files:**
- Modify: `empty.c`
- Do not modify: `empty.syscfg`

**Interfaces:**
- Consumes: `CarMode_Init`, `Buzzer_NotifyInitComplete`.

- [ ] **Step 1: Integrate initialization**

Initialize mode to NONE before OLED first render. Replace the blocking toggle loop with `Buzzer_NotifyInitComplete()` after the 1 ms timer is running.

- [ ] **Step 2: Run all host tests**

Rebuild every test affected by mode, key, buzzer, OLED, lap, race control, and tracking behavior with `-Wall -Wextra -Werror`; execute all binaries.

- [ ] **Step 3: Run SysConfig generation check**

Generate from the unchanged `empty.syscfg` and compare its hash/status before and after.

- [ ] **Step 4: Run TI/CCS forced full build**

Update only the local verification make harness to include `BSP/Mode`, then run:
`E:/ti/ccstheia151/ccs/utils/bin/gmake.exe -B -f .tmp/ccs_full_build.mk all`

Expected: successful link, zero new errors and warnings.

- [ ] **Step 5: Review scope**

Run `git diff --check`, list modified files, and confirm `empty.syscfg`, PID constants, steering branches, finish constants, and hardware pin assignments are unchanged.
