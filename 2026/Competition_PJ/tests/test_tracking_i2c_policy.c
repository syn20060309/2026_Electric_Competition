#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define DL_I2C_CONTROLLER_STATUS_ERROR             0x01U
#define DL_I2C_CONTROLLER_STATUS_ARBITRATION_LOST  0x02U
#define DL_I2C_CONTROLLER_STATUS_IDLE              0x04U
#define DL_I2C_CONTROLLER_STATUS_BUSY_BUS          0x08U

#include "tracking_i2c_policy.h"

static void test_idle_error_is_failure_not_ready(void)
{
    uint32_t status =
        DL_I2C_CONTROLLER_STATUS_IDLE |
        DL_I2C_CONTROLLER_STATUS_ERROR;

    assert(Tracking_I2CStatusFailed(status));
    assert(!Tracking_I2CStatusReady(status, false));
}

static void test_arbitration_loss_is_failure_not_ready(void)
{
    uint32_t status =
        DL_I2C_CONTROLLER_STATUS_IDLE |
        DL_I2C_CONTROLLER_STATUS_ARBITRATION_LOST;

    assert(Tracking_I2CStatusFailed(status));
    assert(!Tracking_I2CStatusReady(status, false));
}

static void test_idle_without_error_is_ready(void)
{
    assert(!Tracking_I2CStatusFailed(
        DL_I2C_CONTROLLER_STATUS_IDLE));
    assert(Tracking_I2CStatusReady(
        DL_I2C_CONTROLLER_STATUS_IDLE, false));
    assert(Tracking_I2CStatusReady(
        DL_I2C_CONTROLLER_STATUS_IDLE, true));
}

static void test_busy_bus_cannot_complete_transfer(void)
{
    uint32_t status =
        DL_I2C_CONTROLLER_STATUS_IDLE |
        DL_I2C_CONTROLLER_STATUS_BUSY_BUS;

    assert(!Tracking_I2CStatusFailed(status));
    assert(Tracking_I2CStatusReady(status, false));
    assert(!Tracking_I2CStatusReady(status, true));
}

int main(void)
{
    test_idle_error_is_failure_not_ready();
    test_arbitration_loss_is_failure_not_ready();
    test_idle_without_error_is_ready();
    test_busy_bus_cannot_complete_transfer();
    return 0;
}
