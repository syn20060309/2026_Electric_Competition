#include "car_mode.h"

static CarMode current_mode = CAR_MODE_NONE;

void CarMode_Init(void)
{
    current_mode = CAR_MODE_NONE;
}

void CarMode_SelectNext(void)
{
    if (current_mode == CAR_MODE_QUESTION_2) {
        current_mode = CAR_MODE_BALL_CONTROL;
    } else {
        current_mode = CAR_MODE_QUESTION_2;
    }
}

CarMode CarMode_GetCurrent(void)
{
    return current_mode;
}

uint16_t CarMode_GetSpeed(void)
{
    if (current_mode == CAR_MODE_QUESTION_2) {
        return SPEED_QUESTION_2;
    }
    if (current_mode == CAR_MODE_BALL_CONTROL) {
        return SPEED_BALL_CONTROL;
    }
    return 0U;
}

bool CarMode_IsSelected(void)
{
    return current_mode != CAR_MODE_NONE;
}

const char *CarMode_GetDisplayName(void)
{
    if (current_mode == CAR_MODE_QUESTION_2) {
        return "Q2 FAST";
    }
    if (current_mode == CAR_MODE_BALL_CONTROL) {
        return "BALL CTRL";
    }
    return "SELECT MODE";
}

const char *CarMode_GetShortName(void)
{
    if (current_mode == CAR_MODE_QUESTION_2) {
        return "Q2";
    }
    if (current_mode == CAR_MODE_BALL_CONTROL) {
        return "BALL";
    }
    return "";
}
