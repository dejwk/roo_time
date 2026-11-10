# Calendar interval resource acceptance

Measured 2026-11-10 with `arm-none-eabi-g++ (xPack GNU Arm Embedded GCC, 64-bit) 9.2.1 20191025 (release) [ARM/arm-9-branch revision 277599]` on the 32-bit
ARM Cortex-M0+ ABI. These are linked bare-metal probes, not a full
firmware or hardware performance benchmark. No flash target was assumed.

Compiler flags: `-std=c++17 -mcpu=cortex-m0plus -mthumb -Os -DNDEBUG -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -fstack-usage`.
Linked with `--specs=nano.specs --specs=nosys.specs -Wl,--gc-sections`
without LTO, using the default newlib startup. Flash is text + data;
BSS excludes automatic stack storage.

## Object sizes

| Type | Before | After |
| --- | ---: | ---: |
| `CalendarBoundary` | — | 16 |
| `CalendarInterval` | — | 12 |
| `FixedTimeZone` | 8 | 8 |
| `OffsetSpan` | — | 24 |
| `RecurringTimeZone` | 24 | 24 |
| `TimeRange` | — | 16 |
| `TimeZone` | 4 | 4 |

No timezone instance storage was added; only vtable entries changed.

## Linked consumers

The same probe uses virtual forward lookup in every variant. Baseline
is `0f235f6`; current adds calendar queries; calendar also calls
hourly enumeration across a fold. The fixed probe uses +05:45; the
recurring probe uses the EU pattern with base +01:00.

| Consumer | Flash bytes | Delta from baseline | BSS bytes |
| --- | ---: | ---: | ---: |
| baseline-fixed | 4384 | +0 | 48 |
| baseline-recurring | 5460 | +0 | 48 |
| current-fixed | 4704 | +320 | 48 |
| current-recurring | 6064 | +604 | 48 |
| calendar-fixed | 6732 | +2348 | 48 |
| calendar-recurring | 8060 | +2600 | 48 |

## Compiler stack frames

`-fstack-usage` reports static frames after inlining, including saved
registers and compiler spills. The 256-byte gate applies to the
enumerator's own frame. These figures do not claim a complete call-tree
bound: provider calls, non-inlined helpers, runtime division/calendar
conversion and visitor frames require additional stack.

| Function | Bytes |
| --- | ---: |
| `int64_t roo_time::{anonymous}::FirstBoundary(int64_t, roo_time::CalendarInterval)` | 48 |
| `static roo_time::CalendarInterval roo_time::CalendarInterval::Days(uint16_t, roo_time::CivilDay)` | 0 |
| `static roo_time::CalendarInterval roo_time::CalendarInterval::Weeks(roo_time::DayOfWeek, uint16_t, roo_time::CivilDay)` | 8 |
| `bool roo_time::CalendarInterval::isValid() const` | 8 |
| `roo_time::CalendarStatus roo_time::EnumerateCalendarBoundaries(roo_time::TimeRange, const roo_time::TimeZone&, roo_time::CalendarInterval, uint32_t&, roo_time::BoundaryVisitor, void*)` | 216 |

The initial enumerator frame was 264 bytes. Decoding only the fields
needed for reverse lookup removed a redundant `DateTime` aggregate.

## Reproduce

Run from the repository root:

```sh
python3 test/measure_calendar_resources.py \
  --compiler /path/to/arm-none-eabi-g++ \
  --output /home/dawidk/.cache/roo_time/calendar-resources
```

The script exports baseline sources, builds six probes serially,
records raw compiler `.su` output and `report.json`, verifies a 32-bit
ABI, unchanged timezone instance sizes, and the enumerator frame gate.
It does not invoke Bazel or alter Bazel caches/resource limits.

`//:calendar_no_allocation_test` links the library statically, wraps
malloc/calloc/realloc and overrides
new/new[] to trap any allocation while exercising all units with fixed,
recurring and synthetic multi-transition zones. Provider conformance
tests verify span/candidate calls never exceed the caller's budget.

## Validation for this implementation

The root-package regression run (`bazel test //:all --test_output=errors`)
passed all 35 test targets. The interval, provider, allocation-trap and guide
example targets also passed with `--config=asan --copt=-fsanitize=undefined
--linkopt=-fsanitize=undefined`. Bazel commands were serialized using the
workspace's stable output root, global disk cache and resource limits.
