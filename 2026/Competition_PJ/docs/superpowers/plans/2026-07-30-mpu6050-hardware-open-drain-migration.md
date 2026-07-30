# MPU6050 Hardware Open-Drain Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move MPU6050 software I2C from PA24/PA25 software-emulated open drain to PA1/PA0 hardware open-drain GPIO and prevent failed sensor initialization from entering the application with all-zero data.

**Architecture:** SysConfig configures PA1 as SCL and PA0 as SDA using the MSPM0 hardware OD structure. The existing software I2C timing remains, but legacy macros directly set, clear, and read GPIO pins; the separate software-open-drain bus module is removed. Startup retries the complete MPU and DMP initialization sequence until both stages succeed.

**Tech Stack:** MSPM0G3507 DriverLib, TI SysConfig, software I2C over hardware open-drain GPIO, InvenSense eMPL/DMP, C11 host tests, TI Arm Clang.

## Global Constraints

- PA1 is MPU6050 SCL and PA0 is MPU6050 SDA, matching the reference project at `E:\my Desktop\2026_Electric_Competition\Competition_PJ`.
- Both pins use `ioStructure = "OD"` and initial value `SET`.
- External pull-ups connect SCL and SDA to 3.3V, not 5V.
- The reference project's MPU GPIO pattern may be reused, but its different tracking, OLED, motor, and task configuration must not be copied.
- Tracking remains hardware I2C0 on PA28/PA31 and OLED remains hardware I2C1 on PA16/PA15.
- Existing acceleration/OLED scheduler, tracking speed, PID, lap-finish, timer, motor, and odometry behavior remain unchanged.
- Generated SysConfig C/H files are not edited manually.

---

### Task 1: Hardware open-drain GPIO macros

**Files:**
- Modify: `empty.syscfg`
- Modify: `BSP/MPU6050/bsp_mpu6050.h`
- Modify: `BSP/MPU6050/bsp_mpu6050.c`
- Delete: `BSP/MPU6050/mpu6050_bus.c`
- Delete: `BSP/MPU6050/mpu6050_bus.h`
- Modify: `tests/test_mpu6050_bus.c`
- Modify: `tests/stubs/ti_msp_dl_config.h`

**Interfaces:**
- Consumes: generated `MPU6050_PORT`, `MPU6050_SCL_PIN`, and `MPU6050_SDA_PIN`.
- Produces: direct legacy macros `SCL(x)`, `SDA(x)`, `SDA_GET()`, `SDA_OUT()`, and `SDA_IN()`.

- [ ] **Step 1: Rewrite the bus test before production code**

Include the real `bsp_mpu6050.h` and test observable GPIO behavior:

```c
SCL(0);
assert((test_gpio_a.output_latch & MPU6050_SCL_PIN) == 0U);
SCL(1);
assert((test_gpio_a.output_latch & MPU6050_SCL_PIN) != 0U);

SDA(0);
SDA_IN();
assert((test_gpio_a.output_enable & MPU6050_SDA_PIN) != 0U);
SDA(1);
assert((test_gpio_a.output_latch & MPU6050_SDA_PIN) != 0U);
```

Set the test pin masks to bit 1 for SCL and bit 0 for SDA. Add a fake
`DL_GPIO_setPins()` that sets the output latch.

- [ ] **Step 2: Run the test to verify RED**

```powershell
gcc -std=c11 -Wall -Wextra -Werror `
  -IBSP/MPU6050 -Itests/stubs `
  tests/test_mpu6050_bus.c `
  -o .tmp/test_mpu6050_hardware_od.exe
```

Expected: link failure because the current macros still call functions from
`mpu6050_bus.c`.

- [ ] **Step 3: Change SysConfig and direct GPIO macros**

Update the MPU6050 GPIO group:

```javascript
GPIO3.associatedPins[0].$name        = "SCL";
GPIO3.associatedPins[0].initialValue = "SET";
GPIO3.associatedPins[0].ioStructure  = "OD";
GPIO3.associatedPins[0].pin.$assign  = "PA1";
GPIO3.associatedPins[1].$name        = "SDA";
GPIO3.associatedPins[1].initialValue = "SET";
GPIO3.associatedPins[1].ioStructure  = "OD";
GPIO3.associatedPins[1].pin.$assign  = "PA0";
```

Remove `mpu6050_bus.h` from `bsp_mpu6050.h`. Define:

```c
#define SDA_OUT() ((void) 0)
#define SDA_IN()  ((void) 0)
#define SDA_GET() \
    (((DL_GPIO_readPins(MPU6050_PORT, MPU6050_SDA_PIN) & \
        MPU6050_SDA_PIN) != 0U) ? 1U : 0U)
#define SDA(x) \
    ((x) ? DL_GPIO_setPins(MPU6050_PORT, MPU6050_SDA_PIN) : \
           DL_GPIO_clearPins(MPU6050_PORT, MPU6050_SDA_PIN))
#define SCL(x) \
    ((x) ? DL_GPIO_setPins(MPU6050_PORT, MPU6050_SCL_PIN) : \
           DL_GPIO_clearPins(MPU6050_PORT, MPU6050_SCL_PIN))
```

In `MPU6050_Init()`, replace software-release function calls with
`SCL(1)` and `SDA(1)`. Delete both `mpu6050_bus` source files.

- [ ] **Step 4: Run the hardware-OD macro test to verify GREEN**

Compile with the Step 2 command and run:

```powershell
.tmp/test_mpu6050_hardware_od.exe
```

Expected: exit code 0.

### Task 2: Initialization failure propagation

**Files:**
- Modify: `BSP/MPU6050/mpu6050_startup.c`
- Modify: `BSP/eMPL/inv_mpu.c`
- Modify: `tests/test_mpu6050_startup.c`

**Interfaces:**
- Consumes: `char MPU6050_Init(void)` and `uint8_t mpu_dmp_init(void)`.
- Produces: `void MPU6050_Startup(void)` that returns only after both stages succeed.

- [ ] **Step 1: Add failing startup tests**

Extend the fakes so MPU initialization has configurable results. Verify:

- first-attempt MPU and DMP success calls each function once without delay;
- two MPU failures cause two 200ms delays and do not call DMP until MPU succeeds;
- two DMP failures cause complete MPU+DMP initialization to run three times and
  two 200ms delays.

- [ ] **Step 2: Run the startup test to verify RED**

```powershell
gcc -std=c11 -Wall -Wextra -Werror `
  -Itests/stubs -IBSP/MPU6050 `
  tests/test_mpu6050_startup.c BSP/MPU6050/mpu6050_startup.c `
  -o .tmp/test_mpu6050_startup_hardware_od.exe
.tmp/test_mpu6050_startup_hardware_od.exe
```

Expected: assertion failure because current startup ignores the MPU return
value and runs MPU initialization only once.

- [ ] **Step 3: Retry the complete initialization sequence**

Implement:

```c
void MPU6050_Startup(void)
{
    while ((MPU6050_Init() != 0) || (mpu_dmp_init() != 0U)) {
        printf("dmp error\r\n");
        delay_ms(200U);
    }
}
```

In `mpu_dmp_init()`, return a nonzero code immediately when `mpu_init()`
fails instead of falling through to `return 0`.

- [ ] **Step 4: Run the startup test to verify GREEN**

Compile and execute with the Step 2 commands. Expected: exit code 0.

### Task 3: Generation and regression verification

**Files:**
- Verify: `empty.syscfg`
- Verify: all current production and host-test files

**Interfaces:**
- Consumes: SysConfig CLI and the existing forced CCS build harness.
- Produces: regenerated configuration evidence and a linked firmware image.

- [ ] **Step 1: Run all host tests**

Compile every host test with `-std=c11 -Wall -Wextra -Werror` and execute
all results. Expected: every existing and updated test exits 0.

- [ ] **Step 2: Run SysConfig in a fresh output directory**

```powershell
E:\ti\sysconfig_1.24.1\sysconfig_cli.bat `
  --product E:\ti\mspm0_sdk_2_05_01_00\.metadata\product.json `
  --script empty.syscfg `
  --output .tmp\syscfg-mpu6050-hardware-od `
  --compiler ticlang
```

Expected: zero errors. Generated code defines SCL on PA1 and SDA on PA0
and initializes both using the hardware high-impedance/open-drain feature.

- [ ] **Step 3: Run a forced TI/CCS full build**

Use:

```powershell
E:\ti\ccstheia151\ccs\utils\bin\gmake.exe `
  -B -f .tmp\ccs_full_build.mk all
```

Expected: all production sources compile and link with no new warnings.
The deleted `mpu6050_bus.c` must no longer appear in the build.

- [ ] **Step 4: Review scope**

Run `git diff --check`, list all changed files, and verify:

- PA28/PA31 tracking and PA16/PA15 OLED mappings are unchanged;
- `IRR_SPEED`, `IRTrack_Trun_KP`, line tracking, lap finish, OLED scheduler,
  and odometry are unchanged;
- `printf_i2c_data()` has no periodic call;
- no generated SysConfig source file was modified.

