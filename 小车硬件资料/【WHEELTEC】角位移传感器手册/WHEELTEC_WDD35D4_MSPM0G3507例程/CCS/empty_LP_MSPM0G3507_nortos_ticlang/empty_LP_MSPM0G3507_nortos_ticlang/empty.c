/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "board.h"

/*
 * 对 PA27 对应的 ADC12 通道执行一次阻塞式单次转换。
 * 返回值为 12 位原始采样结果，范围为 0~4095。
 */
static uint16_t ADC_ReadPA27(void)
{
    uint16_t adc_value;

    /* 清除上次转换完成标志，防止误读旧的转换结果。 */
    DL_ADC12_clearInterruptStatus(
        ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_startConversion(ADC12_0_INST);

    /* 轮询等待 ADCMEM0 装载新结果。 */
    while ((DL_ADC12_getRawInterruptStatus(
                ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED)) == 0U) {
    }

    adc_value = DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_0);

    /* 手动掉电单次转换模式下，需要重新置位 ENC，供下一次转换使用。 */
    DL_ADC12_enableConversions(ADC12_0_INST);

    return adc_value;
}

int main(void)
{
    uint16_t adc_value;
    uint32_t voltage_mv;

    /* 初始化由 SysConfig 生成的时钟、GPIO、UART、ADC 等外设配置。 */
    SYSCFG_DL_init();
    /* 初始化 OLED 控制器并清屏。 */
    OLED_Init();
    printf("ADC PA27 start, UART0 115200 8N1\r\n");

    while (1) 
    {
        adc_value = ADC_ReadPA27();
        /*
         * 按 3.3 V 满量程将 12 位 ADC 码值换算为毫伏。
         * 加 2047 后再除以 4095，用于实现整数计算的四舍五入。
         */
        voltage_mv = ((uint32_t) adc_value * 3300U + 2047U) / 4095U;

        /* 先清空显存，再组织一帧完整内容，最后一次性刷新到 OLED。 */
        memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
        OLED_ShowString(0, 0, (const uint8_t *) "ADC_PIN: PA27");
        OLED_ShowString(0, 16, (const uint8_t *) "ADC_Value:");
        OLED_ShowNumber(88, 16, adc_value, 4, 12);
        OLED_ShowString(0, 32, (const uint8_t *) "Voltage:");
        OLED_ShowNumber(72, 32, voltage_mv, 4, 12);
        OLED_ShowString(104, 32, (const uint8_t *) "mV");
        OLED_Refresh_Gram();

        printf("PA27 ADC=%u, Voltage=%u.%03u V\r\n",
            (unsigned int) adc_value,
            (unsigned int) (voltage_mv / 1000U),
            (unsigned int) (voltage_mv % 1000U));

        /* 按主循环调用次数分频翻转 LED，并控制采样/显示刷新间隔。 */
        LED_Flash(10);
        delay_ms(100);
    }
}
