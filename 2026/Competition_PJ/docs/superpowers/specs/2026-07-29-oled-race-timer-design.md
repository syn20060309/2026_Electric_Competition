# MSPM0G3507 OLED Race Timer Design

## Goal

Add a hardware-I2C SSD1306 display to the existing MSPM0G3507 line-following
car. The display starts timing from zero in the same debounced K1 event that
starts the car, stops and freezes when K1 stops the car, and restarts from zero
on the next start event. OLED failures must not prevent the car from running.

## Existing System

- `BSP/Key/key.c` owns the non-blocking K1 debounce state machine. A short or
  long press event toggles `g_LinePortal_flag`; the same event stops the motors
  when the flag becomes false.
- `g_LinePortal_flag` is defined in `empty.c`. The main loop calls
  `LineWalking()` only while the flag is true.
- `Get_Time()` returns `systick_counter`. TIMG0 is configured as a 1 ms
  periodic timer and increments the counter in `TIMER_0_INST_IRQHandler`, so
  the value is an unsigned millisecond uptime counter.
- The eight-channel tracking sensor uses hardware I2C1 on PA16/PA15 at
  400 kHz and address `0x12`.
- UART0 uses PA10/PA11, motor UART1 uses PB6/PB7, K1 uses PA2, and PA0/PA1 are
  currently unused. There is no PA0/PA1 UART conflict.
- The project has no OLED driver and no reliable lap-finish or A-point stop
  flag. No unverified finish-detection algorithm will be added.

## Selected Approach

Use three focused units:

1. A race-timer module reusing `Get_Time()`.
2. A generic SSD1306 driver with a framebuffer, bounded hardware-I2C
   transactions, and dirty-region updates.
3. A small OLED UI task that maps race-timer state to display content.

This keeps the key state machine and line-following loop intact. A fully
interrupt-driven or DMA OLED transport is unnecessary at the required 100 ms
refresh interval, while putting all state and display logic in `empty.c` would
make the main loop difficult to test and maintain.

## Hardware and SysConfig

Add a second I2C SysConfig instance named `OLED`:

- Peripheral: I2C0 controller
- Bus speed: Standard, 100 kHz
- SDA: PA0
- SCL: PA1

Keep the existing `Sensor` I2C1 instance and every other peripheral assignment
unchanged. DriverLib receives the unshifted 7-bit target address, so the OLED
driver passes `0x3C` directly to `DL_I2C_startControllerTransfer`.

The physical wiring is:

| OLED pin | MSPM0G3507 pin | Function |
| --- | --- | --- |
| GND | GND | Ground |
| VDD | 3.3V | Power |
| SCK | PA1 | I2C0 SCL |
| SDA | PA0 | I2C0 SDA |

## Race Timer

Create `BSP/Timer/race_timer.c` and `race_timer.h` with:

```c
void RaceTimer_Start(void);
void RaceTimer_Stop(void);
void RaceTimer_Reset(void);
uint32_t RaceTimer_GetElapsedMs(void);
bool RaceTimer_IsRunning(void);
```

The module stores a start timestamp, a frozen elapsed value, and an explicit
state:

```c
typedef enum {
    RACE_TIMER_IDLE,
    RACE_TIMER_RUNNING,
    RACE_TIMER_STOPPED
} RaceTimerState;
```

`RaceTimer_Start()` records `Get_Time()`, clears elapsed time, and changes the
state to `RACE_TIMER_RUNNING`. `RaceTimer_Stop()` captures the unsigned
difference between the current tick and start tick before changing the state
to `RACE_TIMER_STOPPED`; stopping while idle has no effect.
`RaceTimer_GetElapsedMs()` returns the current unsigned difference while
running and the captured value while stopped. `RaceTimer_GetState()` exposes
the three states so the UI can distinguish power-on idle from a completed
manual run.

In the existing `KEY_EVENT_SHORT` / `KEY_EVENT_LONG` branch, first toggle
`g_LinePortal_flag`, then:

- call `RaceTimer_Start()` when the flag becomes true;
- call `RaceTimer_Stop()` and stop the motors when the flag becomes false.

Because this code runs only after the existing debounce state machine emits one
event, holding K1 cannot repeatedly reset the timer.

## SSD1306 Driver

Create `BSP/OLED/oled.c`, `oled.h`, `oled_font.c`, and `oled_font.h`.

The driver targets an SSD1306-compatible 128×64 I2C OLED:

```c
#define OLED_I2C_ADDRESS 0x3C
```

It owns a 1024-byte framebuffer equivalent to `OLED_GRAM[128][8]` and exposes:

```c
void OLED_Init(void);
bool OLED_IsReady(void);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_ShowChar(uint8_t x, uint8_t y, char ch);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str);
void OLED_ShowTime(uint32_t elapsed_ms);
bool OLED_WriteCommand(uint8_t command);
bool OLED_WriteData(const uint8_t *data, uint16_t length);
```

The font covers printable ASCII needed by the three screens. Unsupported
characters render as a space.

Each transport operation:

1. waits for an idle controller with a millisecond deadline;
2. flushes stale TX data;
3. writes the SSD1306 control byte and payload through the controller FIFO;
4. starts one transfer using the unshifted 7-bit address;
5. refills the FIFO as space becomes available;
6. checks BUSY, IDLE, and ERROR until completion or timeout;
7. resets/flushes the controller transfer state on failure and returns false.

There are no unbounded wait loops. A failed initialization leaves the internal
ready flag false; later OLED UI calls return immediately and never block car
operation.

Initial setup may clear and update the full display before racing begins.
During racing, drawing tracks dirty page column ranges. `OLED_Update()` sends
only dirty ranges, so changing a time digit does not retransmit all 1024 bytes.

## OLED UI Task

Create `BSP/OLED/oled_task.c` and `oled_task.h`:

```c
void OLED_TaskInit(void);
void OLED_Task(uint32_t now_ms);
```

`OLED_TaskInit()` calls `OLED_Init()` and, only when initialization succeeds,
draws:

```text
LINE CAR
PRESS KEY
TIME:00.00s
```

`OLED_Task()` is called from the main loop and returns immediately when the
OLED is unavailable. It uses unsigned time differences and a 100 ms refresh
interval:

```c
#define OLED_REFRESH_INTERVAL_MS 100U
```

The task detects race-timer state transitions:

- Running: display `RUNNING` and the live elapsed time.
- Stopped after at least one start: display `STOPPED` and the frozen time.
- Never started: retain the startup screen.

Time is formatted to centisecond resolution as `TIME:%02lu.%02lus`. Values
longer than two seconds digits remain visible rather than wrapping at 99
seconds. Formatting arguments are explicitly cast to the type required by the
format string.

The task performs no delays and no OLED work occurs in timer, GPIO, UART, or
motor interrupts. Its bounded I2C transfers send only changed glyph regions to
limit interference with `LineWalking()`.

## Initialization and Main-Loop Integration

Retain `USART_Init()` as the existing owner of `SYSCFG_DL_init()`. Keep the
existing timer interrupt setup and timer start. Call `OLED_TaskInit()` only
after TIMG0 has started, because OLED timeout deadlines use `Get_Time()`.

The main loop becomes conceptually:

```c
while (1)
{
    uint32_t now = Get_Time();

    /* Existing sensor reporting remains unchanged. */
    Key_Handle();
    OLED_Task(now);

    if (g_LinePortal_flag)
    {
        LineWalking();
    }
}
```

No line-following, motor, buzzer, LED, serial, or sensor behavior is removed.

## Build Integration

Add `BSP/OLED` to the compiler include paths in `.cproject`. CCS managed build
discovery will compile the new C sources under the project directory. SysConfig
will generate the `OLED_INST` definitions from `empty.syscfg`; generated
`ti_msp_dl_config.c/.h` remain build outputs rather than hand-edited sources.

## Error Handling

- OLED not connected or NACK: initialization times out, marks the display
  unavailable, and the car continues.
- I2C bus stuck busy: the current operation returns false at its deadline and
  resets the controller transfer state.
- Display write failure after successful initialization: mark the display
  unavailable to prevent repeated delays in the race loop.
- Timer wraparound: all elapsed and scheduling calculations use unsigned
  subtraction.
- No finish detector: manual K1 stop freezes the time and shows `STOPPED`;
  `FINISH` is reserved for a future verified lap-completion signal.

## Verification

Automated checks will cover:

- race timer starts at zero, advances in milliseconds, freezes on stop, and
  restarts from zero;
- unsigned wraparound behavior;
- K1 event integration calls start and stop once per emitted event;
- time conversion and output at representative values;
- source checks for I2C0/PA0/PA1, address `0x3C`, bounded wait conditions, and
  unchanged existing peripheral assignments;
- successful SysConfig generation without pin conflicts;
- a complete CCS build with no new warnings or errors.

Hardware verification remains necessary for electrical wiring, SSD1306
compatibility, visual output, behavior with the OLED disconnected, and any
effect of 100 ms partial updates on real line-following performance.
