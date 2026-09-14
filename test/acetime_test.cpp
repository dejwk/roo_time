#include "roo_time/acetime.h"

int main() {
  using namespace roo_time;

  // Verifies the optional adapter maps AceTime's total offset to UtcOffset.
  AceTimeZone zone(ace_time::TimeZone(-19800));
  if (zone.resolveOffset(WallTime()).offset() != Minutes(-330)) return 1;
  if (zone.resolveOffset(WallTime(Micros(-1))).offset() != Minutes(-330))
    return 2;
  return 0;
}
