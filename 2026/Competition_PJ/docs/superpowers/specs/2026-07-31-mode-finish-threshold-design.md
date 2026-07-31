# Mode-Specific Finish Threshold Design

## Goal

Use a different earliest finish-detection time for each selected car mode:

- `CAR_MODE_QUESTION_2`: 17,500 ms
- `CAR_MODE_BALL_CONTROL`: 25,000 ms

These values gate only normal finish recognition. The 35,000 ms safety timeout remains unchanged.

## Design

`CarMode` owns the mode-specific configuration and exposes the selected mode's finish threshold. When `Car_StartSelectedMode()` starts a race, it passes that threshold to `LapFinish_Start()`.

`LapFinish` stores the threshold for the lifetime of that race. Finish detection compares elapsed race time against this stored value. This prevents a threshold from changing in the middle of a run and keeps the lap detector independent of the global mode state.

The existing finish rules remain unchanged:

- the start line must first be cleared;
- at least five tracking sensors must detect black;
- the finish pattern must be confirmed by two valid samples;
- invalid sensor samples reset finish confirmation;
- the safety timeout remains 35,000 ms.

`CAR_MODE_NONE` returns a zero threshold, but the existing guarded start prevents a race from starting without a selected mode.

## Interfaces

- Add `FINISH_MIN_TIME_QUESTION_2_MS` and `FINISH_MIN_TIME_BALL_CONTROL_MS` to `car_mode.h`.
- Add `CarMode_GetFinishMinTimeMs()` to return the selected mode's threshold.
- Change `LapFinish_Start(now_ms)` to `LapFinish_Start(now_ms, finish_min_time_ms)`.
- Add a read-only lap-finish threshold getter only if required by existing debug output or tests.

## Test Coverage

- Q2 rejects a finish pattern at 17,499 ms and permits confirmation starting at 17,500 ms.
- BALL rejects a finish pattern at 24,999 ms and permits confirmation starting at 25,000 ms.
- Starting a new race after switching modes captures the new threshold.
- Existing five-sensor, confirmation-count, invalid-sample, abort, timer, OLED, and timeout tests continue to pass.
- SysConfig remains unchanged and the TI/CCS full build must link without new compiler warnings.

## Out of Scope

- Speed values, PID parameters, steering logic, OLED layout, key behavior, buzzer patterns, hardware pins, SysConfig, and the 35-second safety timeout are not changed.
