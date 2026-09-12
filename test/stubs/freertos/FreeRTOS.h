#pragma once
#include <stdint.h>
#define configTICK_RATE_HZ 100
#ifdef TEST_16BIT_TICKS
using TickType_t = uint16_t;
#define portMAX_DELAY UINT16_MAX
#else
using TickType_t = uint32_t;
#define portMAX_DELAY UINT32_MAX
#endif
#define pdMS_TO_TICKS(ms) ((uint64_t)(ms) * configTICK_RATE_HZ / 1000)
