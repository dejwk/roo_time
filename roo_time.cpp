#include "roo_time.h"

extern "C" {
int64_t esp_timer_get_time();
}

#include "esp_attr.h"

namespace roo_time {

static int64_t last_reading = 0;

// Offset to make sure the time is monotone.
static int64_t offset = 0;


const Uptime IRAM_ATTR Uptime::Now() { 
  int64_t now = esp_timer_get_time() + offset;
  int64_t diff = last_reading - now;
  if (diff > 0) {
    offset += diff;
    now += diff;
  }
  last_reading = now;
  return Uptime(now);
}

}  // namespace roo_time