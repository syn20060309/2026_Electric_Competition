#ifndef LAP_FINISH_H
#define LAP_FINISH_H

#include <stdbool.h>
#include <stdint.h>

#define FINISH_MIN_TIME_MS              14000U
#define START_LINE_CLEAR_CONFIRM_MS       100U
#define FINISH_ACTIVE_COUNT_THRESHOLD       6U
#define FINISH_CONFIRM_SAMPLE_COUNT          2U
#define LAP_SAFETY_TIMEOUT_MS           35000U

#define ENABLE_K1_MANUAL_STOP_DEBUG         0
#define ENABLE_K1_EMERGENCY_STOP            1
#define LAP_FINISH_DEBUG                     0

typedef enum {
    LAP_STATE_IDLE = 0,
    LAP_STATE_LEAVING_START,
    LAP_STATE_RUNNING,
    LAP_STATE_FINISH_DETECTED,
    LAP_STATE_FINISHED,
    LAP_STATE_ABORTED,
    LAP_STATE_TIMEOUT
} LapFinish_State;

typedef enum {
    LAP_FINISH_EVENT_NONE = 0,
    LAP_FINISH_EVENT_FINISH
} LapFinish_Event;

void LapFinish_Init(void);
void LapFinish_Reset(void);
void LapFinish_Start(uint32_t now_ms);
bool LapFinish_CheckTimeout(uint32_t elapsed_ms);
LapFinish_Event LapFinish_Update(uint32_t now_ms, uint32_t elapsed_ms,
    bool sensor_data_valid, uint8_t active_mask);
LapFinish_State LapFinish_GetState(void);
void LapFinish_MarkFinished(void);
void LapFinish_MarkAborted(void);
void LapFinish_MarkTimeout(void);
bool LapFinish_IsFinished(void);
bool LapFinish_IsAborted(void);
bool LapFinish_StartLineCleared(void);
uint8_t LapFinish_GetConfirmCount(void);

#endif
