#include "oled.h"

#include <limits.h>
#include <string.h>

#include "oled_font.h"
#include "ti_msp_dl_config.h"

#define OLED_CONTROL_COMMAND 0x00U
#define OLED_CONTROL_DATA 0x40U
#define OLED_I2C_TIMEOUT_MS 25U
#define OLED_I2C_LOOP_GUARD 500000U

static uint8_t OLED_GRAM[OLED_PAGE_COUNT][OLED_WIDTH];
static uint8_t dirty_first[OLED_PAGE_COUNT];
static uint8_t dirty_last[OLED_PAGE_COUNT];
static bool oled_ready;

extern uint32_t Get_Time(void);

static void OLED_TransportAbort(void)
{
    DL_I2C_resetControllerTransfer(OLED_INST);
    DL_I2C_flushControllerTXFIFO(OLED_INST);
}

static bool OLED_DeadlineExpired(uint32_t start_ms, uint32_t *loop_guard)
{
    if (*loop_guard == 0U) {
        return true;
    }

    (*loop_guard)--;
    return (uint32_t) (Get_Time() - start_ms) >= OLED_I2C_TIMEOUT_MS;
}

static bool OLED_WaitForIdle(void)
{
    uint32_t start_ms = Get_Time();
    uint32_t loop_guard = OLED_I2C_LOOP_GUARD;

    while ((DL_I2C_getControllerStatus(OLED_INST) &
            DL_I2C_CONTROLLER_STATUS_IDLE) == 0U) {
        if ((DL_I2C_getControllerStatus(OLED_INST) &
                DL_I2C_CONTROLLER_STATUS_ERROR) != 0U ||
            OLED_DeadlineExpired(start_ms, &loop_guard)) {
            OLED_TransportAbort();
            return false;
        }
    }

    return true;
}

static bool OLED_WriteControl(
    uint8_t control, const uint8_t *data, uint16_t length)
{
    uint16_t bytes_queued;
    uint32_t start_ms;
    uint32_t loop_guard = OLED_I2C_LOOP_GUARD;

    if ((data == NULL) || (length == 0U) || (length == UINT16_MAX)) {
        return false;
    }

    if (!OLED_WaitForIdle()) {
        return false;
    }

    DL_I2C_flushControllerTXFIFO(OLED_INST);
    (void) DL_I2C_fillControllerTXFIFO(OLED_INST, &control, 1U);
    bytes_queued =
        DL_I2C_fillControllerTXFIFO(OLED_INST, data, length);

    DL_I2C_startControllerTransfer(OLED_INST, OLED_I2C_ADDRESS,
        DL_I2C_CONTROLLER_DIRECTION_TX, (uint16_t) (length + 1U));

    start_ms = Get_Time();
    while (bytes_queued < length) {
        uint32_t status = DL_I2C_getControllerStatus(OLED_INST);

        if ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U ||
            OLED_DeadlineExpired(start_ms, &loop_guard)) {
            OLED_TransportAbort();
            return false;
        }

        if (!DL_I2C_isControllerTXFIFOFull(OLED_INST)) {
            DL_I2C_transmitControllerData(
                OLED_INST, data[bytes_queued]);
            bytes_queued++;
        }
    }

    while ((DL_I2C_getControllerStatus(OLED_INST) &
                DL_I2C_CONTROLLER_STATUS_BUSY_BUS) != 0U ||
        (DL_I2C_getControllerStatus(OLED_INST) &
                DL_I2C_CONTROLLER_STATUS_IDLE) == 0U) {
        uint32_t status = DL_I2C_getControllerStatus(OLED_INST);

        if ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U ||
            OLED_DeadlineExpired(start_ms, &loop_guard)) {
            OLED_TransportAbort();
            return false;
        }
    }

    if ((DL_I2C_getControllerStatus(OLED_INST) &
            DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
        OLED_TransportAbort();
        return false;
    }

    return true;
}

static void OLED_MarkDirty(
    uint8_t page, uint8_t first_column, uint8_t last_column)
{
    if (first_column < dirty_first[page]) {
        dirty_first[page] = first_column;
    }
    if (last_column > dirty_last[page]) {
        dirty_last[page] = last_column;
    }
}

bool OLED_WriteCommand(uint8_t command)
{
    bool success = OLED_WriteControl(OLED_CONTROL_COMMAND, &command, 1U);

    if (!success) {
        oled_ready = false;
    }
    return success;
}

bool OLED_WriteData(const uint8_t *data, uint16_t length)
{
    bool success = OLED_WriteControl(OLED_CONTROL_DATA, data, length);

    if (!success) {
        oled_ready = false;
    }
    return success;
}

void OLED_Init(void)
{
    static const uint8_t init_commands[] = {
        0xAEU, 0xD5U, 0x80U, 0xA8U, 0x3FU, 0xD3U, 0x00U, 0x40U,
        0x8DU, 0x14U, 0x20U, 0x02U, 0xA1U, 0xC8U, 0xDAU, 0x12U,
        0x81U, 0xCFU, 0xD9U, 0xF1U, 0xDBU, 0x40U, 0xA4U, 0xA6U,
        0xAFU
    };

    oled_ready = false;
    OLED_TransportAbort();

    if (!OLED_WriteControl(OLED_CONTROL_COMMAND, init_commands,
            (uint16_t) sizeof(init_commands))) {
        return;
    }

    oled_ready = true;
    OLED_Clear();
    OLED_Update();
}

bool OLED_IsReady(void)
{
    return oled_ready;
}

void OLED_Clear(void)
{
    uint8_t page;

    (void) memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
    for (page = 0U; page < OLED_PAGE_COUNT; page++) {
        dirty_first[page] = 0U;
        dirty_last[page] = OLED_WIDTH - 1U;
    }
}

void OLED_ShowChar(uint8_t x, uint8_t y, char ch)
{
    const uint8_t *glyph;
    uint8_t column;
    uint8_t character = (uint8_t) ch;

    if ((x >= OLED_WIDTH) || (y >= OLED_PAGE_COUNT)) {
        return;
    }

    if ((character < OLED_FONT_FIRST_CHAR) ||
        (character > OLED_FONT_LAST_CHAR)) {
        character = (uint8_t) ' ';
    }
    glyph = OLED_Font6x8[character - OLED_FONT_FIRST_CHAR];

    for (column = 0U;
         (column < OLED_FONT_GLYPH_WIDTH) &&
         ((uint16_t) x + column < OLED_WIDTH);
         column++) {
        uint8_t target_column = x + column;

        if (OLED_GRAM[y][target_column] != glyph[column]) {
            OLED_GRAM[y][target_column] = glyph[column];
            OLED_MarkDirty(y, target_column, target_column);
        }
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *str)
{
    if (str == NULL) {
        return;
    }

    while ((*str != '\0') && (x < OLED_WIDTH)) {
        OLED_ShowChar(x, y, *str);
        x = (uint8_t) (x + OLED_FONT_GLYPH_WIDTH);
        str++;
    }
}

void OLED_Update(void)
{
    uint8_t page;

    if (!oled_ready) {
        return;
    }

    for (page = 0U; page < OLED_PAGE_COUNT; page++) {
        uint8_t first = dirty_first[page];
        uint8_t last = dirty_last[page];
        uint8_t commands[3];

        if ((first >= OLED_WIDTH) || (last < first)) {
            continue;
        }

        commands[0] = (uint8_t) (0xB0U | page);
        commands[1] = (uint8_t) (first & 0x0FU);
        commands[2] = (uint8_t) (0x10U | (first >> 4U));

        if (!OLED_WriteControl(
                OLED_CONTROL_COMMAND, commands, sizeof(commands)) ||
            !OLED_WriteData(
                &OLED_GRAM[page][first], (uint16_t) (last - first + 1U))) {
            oled_ready = false;
            return;
        }

        dirty_first[page] = OLED_WIDTH;
        dirty_last[page] = 0U;
    }
}
