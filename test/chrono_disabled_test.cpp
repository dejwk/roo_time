#define ROO_TIME_HAS_CHRONO 0
#include "roo_time/chrono.h"
static_assert(!ROO_TIME_HAS_CHRONO, "Adapter can be disabled without chrono");
int main() {
  const roo_time::Duration value = roo_time::Seconds(2);
  return value.inMillis() == 2000 ? 0 : 1;
}
