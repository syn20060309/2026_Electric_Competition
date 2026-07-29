#ifndef TRACKING_I2C_POLICY_H
#define TRACKING_I2C_POLICY_H

#include <stdbool.h>
#include <stdint.h>

static inline bool Tracking_I2CStatusFailed(uint32_t status)
{
    return (status &
        (DL_I2C_CONTROLLER_STATUS_ERROR |
         DL_I2C_CONTROLLER_STATUS_ARBITRATION_LOST)) != 0U;
}

static inline bool Tracking_I2CStatusReady(
    uint32_t status, bool require_bus_idle)
{
    if (Tracking_I2CStatusFailed(status) ||
        ((status & DL_I2C_CONTROLLER_STATUS_IDLE) == 0U)) {
        return false;
    }

    return !require_bus_idle ||
        ((status & DL_I2C_CONTROLLER_STATUS_BUSY_BUS) == 0U);
}

#endif
