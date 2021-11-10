# roo_time
Arduino-compliant ESP32 (etc.) library for basic management of elapsed time, wall time, and date time with multi-timezone support.
Provides type safety around time intervals and different time units, guarding against common programming errors like confusing time
units, or confusing 'timestamps' with 'intervals'.

When used with Espressif chips, uses microsecond precision, and stores the time as int64_t
(in other words, it will not overflow on you).

I wrote this library because I couldn't find another that supported this full range of features, while working seamlessly with
various RTC devices. In particular, with DS3231, I wanted to keep the time as UTC, and deal with daylight savings conversions
outside of that. But timezone support is not commonly found in Arduino time libraries.

## Compatibility

This library is extensively tested on ESP32 with the Arduino framework. That said, it should also work without changes on other Espressif chips, and with the Espressif framework instead of Arduino. (This library does not have any explicit dependency on Arduino).

The library is written in standard C++, and the only platform-dependent function is the one behind Uptime::Now(). It should work out-of-the-box on arbitrary Arduino platforms, although it wasn't explicitly tested beyond ESP32.

## Measuring elapsed time

Example usage:

```cpp
#include "roo_time.h"

using namespace roo_time;

void main() {
  Uptime now = Uptime::Now();  // Carries microseconds since Epoch.
  foo(now.inMillis());         // Conveniently convert to various time units, as needed.
  now += Hours(2);             // Basic arithmetics and convenience construction.
  if (now > Hours(2))          // Compile error: don't conflate time instant with time interval.
  
  // Measuring elapsed time
  Uptime start = Uptime::Now();
  // ... do something
  Interval elapsed = Uptime::Now() - start;
  if (elapsed > Minutes(2)) {  // This is now OK.
    // ...
  }
}
```

## Measuring wall time

The library works well with device-specific libraries, via the base abstraction of a 'WallTimeClock'. For example, when use with DS3231, you need to write the following adapter:

```cpp
#include "DS3231.h"
#include "roo_time.h"

using namespace roo_time;

class Clock : public WallTimeClock {
 public:
  void begin() { rtc_.begin(); }
  
  WallTime now() const override {
    auto ds = rtc_.getDateTime();
    // Time zone that you use for storing time in your DS3231. I like UTC,
    // but you may also keep it in the local time.
    TimeZone tz = timezone::UTC;
    DateTime dt(ds.year, ds.month, ds.day, ds.hour, ds.minute, ds.second, 0, tz);
    return dt.wallTime();
  }
  
 private:
  DS3231 rtc_;
};

```

Once you have such an adapter, you can use it like this:

```cpp
Clock clock;

void setup() {
  clock.begin();
}

void main() {
  WallTime now = clock.now();
  WallTime tomorrow = now + Hours(24);
  int64_t seconds_since_epoch = now.sinceEpoch().toSeconds();
}
```

## Date / time conversion

You can specify datetimes, and convert them from and to wall time:

```cpp
// Get a wall time of a specified calendar datetime.
DateTime independence_day(2021, 7, 4, TimeZone(Hours(-7)));
WallTime wt = independence_day.wallTime();

// Get current time in a specified timezone.
DateTime now(clock.now(), TimeZone(Hours(2));
foo(now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

if (now.dayOfWeek() == FRIDAY) { /* I like Fridays! */ }

```

## Timezones and daylight savings

Timezone is just a type-safe interval wrapper:

```cpp
static const Timezone CEST(Hours(2)); 
```

Daylight saving rules are not explicitly supported, because they are very complicated and change
often. It is, however, reasonably simple to implement the logic yourself. For example, in Poland,
summer time begins at 2AM local time on the last Sunday of March, and it ends at 3AM local time
on the last Sunday of October. The appropriate daylight-savings-aware clock looks like this:

```cpp
Interval utcOffset(WallTime t) {
  int16_t y = DateTime(t, timezone::UTC).year();
  // Figure out the day of the week of the last day of March that year.
  DateTime mar31(y, 3, 31, timezone::UTC);
  // Figure out the down-offset from mar31 to the 2AM last Sunday of March.
  // Keep in mind that 2AM is 1AM UTC.
  DayOfWeek march31dow = mar31.dayOfWeek();
  WallTime summerStart = mar31.wallTime() - Hours(24 * march31dow) + Hours(1);
  // Similar calculation for the winter time. Note that 3AM is now 1AM UTC.
  DateTime oct31(y, 10, 31, timezone::UTC);
  DayOfWeek oct31dow = oct31.dayOfWeek();
  WallTime summerEnd = oct31.wallTime() - Hours(24 * oct31dow) + Hours(1);
  // Now, see if the specified time point is within the summer time range.
  return t >= summerStart && t < summerEnd ? Hours(2) : Hours(3);
}

class DSTWatch {
 public:
  DSTWatch(Clock* clock) : clock_(clock) {}
 
  DateTime nowLocal() {
    WallTime t = clock_->now();
    return DateTime(t, TimeZone(utcOffset(t)));
  }
  
 private:
  Clock* clock_;
};
```

## Type safety

The library will protect you from making common mistakes, such as mixing up time units,
mixing up uptime (i.e. the time since the device is running) with wall time (i.e. duration
since Epoch), and mixing up intervals with time points:

```cpp
clock.now() - Uptime::Now();  // ERROR: can't mix up wall time and uptime.
clock.now() - (Uptime::Now() - Uptime::Start());  // Now OK; explicitly converted to an interval.
                                                  // Returns the wall time of last restart.
Uptime::Now() + 20;           // ERROR: 20 of what?
Uptime::Now() + Seconds(20);  // Now OK.
```

## Performance

The Uptime and WallTime classes are trivial wrappers around int64.
For such classes, construction/destruction has zero cost, as it is completely
optimized away. The compiler will generate code that will look exactly as if
you directly operated on the int64.

## Program size overhead

The compiler is good at omitting stuff you don't use. For example, if you never call any
date conversion function, it will have zero effect on your binary size. And, because
of aggressive optimization mentioned above, the cost of using basic functionality
of WallTime and Uptime is essentially zero in comparison to the equivalent code using
integer types directly.
