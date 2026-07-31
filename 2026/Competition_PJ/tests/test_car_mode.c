#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "car_mode.h"

static void test_initializes_without_a_selected_mode(void)
{
    CarMode_Init();

    assert(CarMode_GetCurrent() == CAR_MODE_NONE);
    assert(CarMode_GetSpeed() == 0U);
    assert(!CarMode_IsSelected());
    assert(strcmp(CarMode_GetDisplayName(), "SELECT MODE") == 0);
    assert(strcmp(CarMode_GetShortName(), "") == 0);
}

static void test_selection_cycles_between_q2_and_ball_after_first_choice(void)
{
    CarMode_Init();

    CarMode_SelectNext();
    assert(CarMode_GetCurrent() == CAR_MODE_QUESTION_2);
    assert(CarMode_GetSpeed() == 335U);
    assert(CarMode_IsSelected());
    assert(strcmp(CarMode_GetDisplayName(), "Q2 FAST") == 0);
    assert(strcmp(CarMode_GetShortName(), "Q2") == 0);

    CarMode_SelectNext();
    assert(CarMode_GetCurrent() == CAR_MODE_BALL_CONTROL);
    assert(CarMode_GetSpeed() == 230U);
    assert(strcmp(CarMode_GetDisplayName(), "BALL CTRL") == 0);
    assert(strcmp(CarMode_GetShortName(), "BALL") == 0);

    CarMode_SelectNext();
    assert(CarMode_GetCurrent() == CAR_MODE_QUESTION_2);
    assert(CarMode_GetSpeed() == 335U);
}

int main(void)
{
    test_initializes_without_a_selected_mode();
    test_selection_cycles_between_q2_and_ball_after_first_choice();
    return 0;
}
