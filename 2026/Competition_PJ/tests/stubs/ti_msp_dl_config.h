#ifndef TEST_TI_MSP_DL_CONFIG_H
#define TEST_TI_MSP_DL_CONFIG_H

#include <stdint.h>

typedef struct {
    uint32_t output_latch;
    uint32_t output_enable;
    uint32_t input_state;
} GPIO_Regs;

extern GPIO_Regs test_gpio_a;

typedef struct {
    uint32_t compare_value;
    uint32_t start_count;
} Timer_Regs;

extern Timer_Regs test_buzzer_timer;

#define MPU6050_PORT       (&test_gpio_a)
#define MPU6050_SCL_PIN    (1UL << 1)
#define MPU6050_SDA_PIN    (1UL << 0)
#define MPU6050_SDA_IOMUX  (1U)
#define KEY_PORT            (&test_gpio_a)
#define KEY_K1_PIN          (1UL << 2)
#define BUZZER_INST          (&test_buzzer_timer)
#define GPIO_BUZZER_C3_IDX   (3U)

static inline void DL_Timer_startCounter(Timer_Regs *timer)
{
    timer->start_count++;
}

static inline void DL_TimerA_setCaptureCompareValue(
    Timer_Regs *timer, uint32_t value, uint32_t index)
{
    (void) index;
    timer->compare_value = value;
}

static inline void DL_GPIO_initDigitalOutput(uint32_t iomux)
{
    (void) iomux;
}

static inline void DL_GPIO_initDigitalInput(uint32_t iomux)
{
    (void) iomux;
    test_gpio_a.output_enable &= ~MPU6050_SDA_PIN;
}

static inline void DL_GPIO_setPins(GPIO_Regs *gpio, uint32_t pins)
{
    gpio->output_latch |= pins;
}

static inline void DL_GPIO_clearPins(GPIO_Regs *gpio, uint32_t pins)
{
    gpio->output_latch &= ~pins;
}

static inline void DL_GPIO_enableOutput(GPIO_Regs *gpio, uint32_t pins)
{
    gpio->output_enable |= pins;
}

static inline void DL_GPIO_disableOutput(GPIO_Regs *gpio, uint32_t pins)
{
    gpio->output_enable &= ~pins;
}

static inline uint32_t DL_GPIO_readPins(GPIO_Regs *gpio, uint32_t pins)
{
    return gpio->input_state & pins;
}

#endif
