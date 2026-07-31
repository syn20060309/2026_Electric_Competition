#ifndef CAR_MODE_H
#define CAR_MODE_H

#include <stdbool.h>
#include <stdint.h>

#define SPEED_QUESTION_2       335U
#define SPEED_BALL_CONTROL     230U

typedef enum {
    CAR_MODE_NONE = 0,
    CAR_MODE_QUESTION_2,
    CAR_MODE_BALL_CONTROL
} CarMode;

void CarMode_Init(void);
void CarMode_SelectNext(void);
CarMode CarMode_GetCurrent(void);
uint16_t CarMode_GetSpeed(void);
bool CarMode_IsSelected(void);
const char *CarMode_GetDisplayName(void);
const char *CarMode_GetShortName(void);

#endif
