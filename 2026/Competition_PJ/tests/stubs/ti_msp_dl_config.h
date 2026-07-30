#ifndef TEST_TI_MSP_DL_CONFIG_H
#define TEST_TI_MSP_DL_CONFIG_H

#include <stdint.h>

typedef struct {
    uint32_t output_latch;
    uint32_t output_enable;
    uint32_t input_state;
} GPIO_Regs;

extern GPIO_Regs test_gpio_a;

#define MPU6050_PORT       (&test_gpio_a)
#define MPU6050_SCL_PIN    (1UL << 1)
#define MPU6050_SDA_PIN    (1UL << 0)

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
