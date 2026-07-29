# A-Line Automatic Stop Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the second K1 manual stop with reliable one-lap A-line detection, automatic motor/timer stop, and FINISH/ABORT/TIMEOUT OLED states.

**Architecture:** A pure `lap_finish` state machine consumes the current race time and the active mask built from the same sensor frame already read by `LineWalking()`. Hardware I2C validity stays in the tracking driver; stopping stays in application integration so the state machine performs no I2C, OLED, motor, or blocking work.

**Tech Stack:** MSPM0 DriverLib C11, TI Arm Clang 4.0.0 LTS, SysConfig 1.24.1, host GCC tests.

## Global Constraints

- A valid A-line sample is exactly eight active sensors: `active_mask == 0xFFU`; six or seven must never finish.
- A-line finish is enabled only after the start line has remained cleared for 100 ms and elapsed race time is at least 14000 ms.
- One valid all-eight sample latches immediately; no multi-sample or dwell requirement is allowed.
- Sensor read failures must never advance finish detection or reuse stale/default data.
- Safety timeout is 35000 ms and is not a normal finish.
- K1 short press starts only; K1 long press aborts while running.
- Reuse the current `LineWalking()` sensor frame; do not add another I2C read.
- Preserve PA0/PA1 OLED I2C0, all other pins, PID constants, speed, tracking decisions, race timer implementation, and OLED transport.

---

### Task 1: Pure lap-finish state machine

**Files:**
- Create: `BSP/Eight_Tracking/lap_finish.h`
- Create: `BSP/Eight_Tracking/lap_finish.c`
- Create: `tests/test_lap_finish.c`

**Interfaces:**
- Consumes: `now_ms`, `elapsed_ms`, `sensor_data_valid`, and `active_mask` from one tracking frame.
- Produces: `LapFinish_Update(...)` event, `LapFinish_GetState()`, start/reset/finish/abort transitions.

- [ ] Write tests covering IDLE, 100 ms start-line-clear confirmation, invalid-frame rejection, 6/7-light rejection, 14 s unlock, one-sample 0xFF latch, 35 s timeout, wraparound, finish latch, abort, and restart.
- [ ] Compile before implementation and verify failure because `lap_finish.c` is absent.
- [ ] Implement the minimal state machine with all timing/configuration macros centralized in `lap_finish.h`.
- [ ] Compile with `-Wall -Wextra -Werror`, run, and verify all assertions pass.

### Task 2: OLED outcome states

**Files:**
- Modify: `BSP/OLED/oled_task.c`
- Modify: `tests/test_oled_task.c`

**Interfaces:**
- Consumes: `LapFinish_GetState()` and `RaceTimer_GetElapsedMs()`.
- Produces: `FINISH`, `ABORT`, or `TIMEOUT` status while retaining IDLE/RUNNING behavior and final time.

- [ ] Extend the OLED fake test to inject lap state and assert each outcome label and retained time.
- [ ] Run the existing host test before production changes and verify the new assertions fail.
- [ ] Select display text from lap state without changing OLED I2C transport or refresh cadence.
- [ ] Run OLED and race-timer host tests with warnings as errors.

### Task 3: Valid single-frame tracking input and automatic stop

**Files:**
- Modify: `BSP/Eight_Tracking/app_irtracking.h`
- Modify: `BSP/Eight_Tracking/app_irtracking.c`

**Interfaces:**
- Consumes: the sensor's raw active-low byte from register `0x30`.
- Produces: `bool deal_IRdata(...)`, active mask where one means LED/black active, and a single automatic-stop event from `LineWalking()`.

- [ ] Add host-testable active-low conversion assertions: raw `0x00 -> 0xFF`, raw `0xC0 -> 0x3F`, and raw `0x80 -> 0x7F`.
- [ ] Verify the conversion test fails before adding the production helper.
- [ ] Change the hardware read to use bounded DriverLib polling, detect status error/NACK through controller error status, abort/flush on failure, and write outputs only after success.
- [ ] Make `LineWalking()` pass the successful frame's mask to `LapFinish_Update()` before the unchanged tracking decision tree.
- [ ] On FINISH or TIMEOUT, clear `g_LinePortal_flag`, send the existing zero motor command, stop `RaceTimer`, mark the final lap state, and return before any nonzero motor command.

### Task 4: K1 start-only and emergency abort

**Files:**
- Modify: `BSP/Key/key.c`

**Interfaces:**
- Consumes: existing debounced `KEY_EVENT_SHORT` and `KEY_EVENT_LONG`.
- Produces: short-press start/restart only when not running; long-press zero-motor abort while running.

- [ ] Route short press through `LapFinish_Reset()`, `RaceTimer_Start()`, `LapFinish_Start(Get_Time())`, and `g_LinePortal_flag = 1`.
- [ ] Ignore short presses while running because `ENABLE_K1_MANUAL_STOP_DEBUG` is zero.
- [ ] Route long press through flag clear, existing zero motor command, `RaceTimer_Stop()`, and `LapFinish_MarkAborted()`.
- [ ] Compile the module with warnings as errors.

### Task 5: Toolchain and acceptance verification

**Files:**
- Modify only if required by compiler/build evidence: `.cproject`, `.tmp/ccs_full_build.mk`

**Interfaces:**
- Consumes: all production sources and `empty.syscfg`.
- Produces: host-test results, generated SysConfig evidence, linked `.out`, and final change report.

- [ ] Run all host tests and confirm zero failures.
- [ ] Run SysConfig generation and confirm no new error/warning or pin changes.
- [ ] Run a forced full TI Arm Clang build and link.
- [ ] Run `git diff --check`, inspect the complete diff, and confirm PID/speed/pins/OLED transport remain unchanged.
- [ ] Report original and new K1 behavior, sensor polarity/mask, full finish condition, stop method, state machine, build results, and remaining hardware checks.
