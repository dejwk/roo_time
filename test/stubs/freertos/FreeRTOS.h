#pragma once
#include <stdint.h>
// Deliberately use a coarse tick to expose sub-tick delays.
#define pdMS_TO_TICKS(ms) ((uint64_t)(ms) * 100 / 1000)
