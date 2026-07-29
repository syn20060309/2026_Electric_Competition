#include "tracking_sample.h"

uint8_t Tracking_BuildActiveMask(
    uint8_t x1, uint8_t x2, uint8_t x3, uint8_t x4,
    uint8_t x5, uint8_t x6, uint8_t x7, uint8_t x8)
{
    uint8_t raw_mask =
        (uint8_t) ((x1 << 7U) |
                   (x2 << 6U) |
                   (x3 << 5U) |
                   (x4 << 4U) |
                   (x5 << 3U) |
                   (x6 << 2U) |
                   (x7 << 1U) |
                   x8);

    return (uint8_t) ~raw_mask;
}
