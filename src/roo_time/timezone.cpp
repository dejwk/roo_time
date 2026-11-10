#include "roo_time/timezone.h"

#include <initializer_list>

namespace roo_time {
namespace {

// Checks selector, minute, and clock-basis ranges without resolving a year.
inline bool ValidTransition(const AnnualTransition& t) {
  return t.date.isValid() && t.minutes_since_midnight <= 1440 &&
         (t.basis == TransitionTimeBasis::kUtc ||
          t.basis == TransitionTimeBasis::kBaseLocal ||
          t.basis == TransitionTimeBasis::kAdjustedLocal);
}

// Converts annual transition fields to an instant using their explicit basis.
WallTime TransitionAt(const AnnualTransition& t, int year, UtcOffset base,
                      UtcOffset adjustment) {
  uint8_t day = 0;
  t.date.resolveDay(year, day);  // Already validated.
  WallTime instant =
      DateTime(year, t.date.month(), day, timezone::UTC).wallTime();
  instant += Minutes(t.minutes_since_midnight);
  if (t.basis != TransitionTimeBasis::kUtc) instant -= base.asDuration();
  if (t.basis == TransitionTimeBasis::kAdjustedLocal) {
    instant -= adjustment.asDuration();
  }
  return instant;
}

WallTime CalendarBegin() { return DateTime(1, 1, 1, timezone::UTC).wallTime(); }

WallTime CalendarEnd() {
  return DateTime(9999, 12, 31, timezone::UTC).wallTime() + Hours(24);
}

// Keeps UTC and every possible local offset inside calendar years 1-9999.
TimeRange SafeCalendarRange(UtcOffset minimum, UtcOffset maximum) {
  const WallTime lower = CalendarBegin();
  const WallTime upper = CalendarEnd();
  return {minimum.inMinutes() < 0 ? lower - minimum.asDuration() : lower,
          maximum.inMinutes() > 0 ? upper - maximum.asDuration() : upper};
}

}  // namespace

CalendarStatus TimeZone::calendarRange(TimeRange&) const {
  return CalendarStatus::kUnsupported;
}

CalendarStatus TimeZone::offsetSpanAt(WallTime, OffsetSpan&) const {
  return CalendarStatus::kUnsupported;
}

CalendarStatus FixedTimeZone::calendarRange(TimeRange& out) const {
  out = SafeCalendarRange(offset_, offset_);
  return CalendarStatus::kOk;
}

CalendarStatus FixedTimeZone::offsetSpanAt(WallTime instant,
                                           OffsetSpan& out) const {
  if (!instant.isSet()) return CalendarStatus::kInvalidArgument;
  const TimeRange range = SafeCalendarRange(offset_, offset_);
  if (instant < range.begin || instant >= range.end) {
    return CalendarStatus::kOutOfRange;
  }
  out = {range, offset_};
  return CalendarStatus::kOk;
}

LocalTimeResolution TimeZone::resolveLocal(CivilDay date,
                                           TimeOfDay time) const {
  if (!date.isValid() || !time.isValid()) {
    return {LocalTimeStatus::kInvalidInput};
  }
  return {LocalTimeStatus::kUnsupported};
}

LocalTimeResolution FixedTimeZone::resolveLocal(CivilDay date,
                                                TimeOfDay time) const {
  DateTime local;
  if (!DateTime::TryFromComponents(date, time, timezone::UTC, local)) {
    return {LocalTimeStatus::kInvalidInput};
  }
  return {LocalTimeStatus::kUnique, local.wallTime() - offset_.asDuration()};
}

bool AnnualDateRule::isValid() const {
  if (month_ < kJanuary || month_ > kDecember) return false;
  if (weekday_ < kSunday || weekday_ > kSaturday) return false;
  const int minimum_days = DaysInMonth(2001, month_);
  switch (kind_) {
    case Kind::kDay:
      return value_ >= 1 && value_ <= minimum_days;
    case Kind::kNth:
      return value_ >= 1 && value_ <= 4;
    case Kind::kLast:
      return true;
    case Kind::kOnOrAfter:
      return value_ >= 1 && value_ + 6 <= minimum_days;
  }
  return false;
}

bool AnnualDateRule::resolveDay(uint16_t year, uint8_t& result) const {
  if (year < 1 || year > 9999 || !isValid()) return false;
  int day;
  if (kind_ == Kind::kDay) {
    day = value_;
  } else {
    const int anchor = kind_ == Kind::kLast        ? DaysInMonth(year, month_)
                       : kind_ == Kind::kOnOrAfter ? value_
                                                   : 1;
    const int weekday = CivilDay::FromYmd(year, month_, anchor).dayOfWeek();
    if (kind_ == Kind::kLast) {
      day = anchor - (weekday - weekday_ + 7) % 7;
    } else {
      day = anchor + (weekday_ - weekday + 7) % 7;
      if (kind_ == Kind::kNth) day += 7 * (value_ - 1);
    }
  }
  result = static_cast<uint8_t>(day);
  return true;
}

RecurringTimeZone::RecurringTimeZone(UtcOffset base, SeasonalRules rules)
    : base_offset_(base), rules_(rules) {
  assert(ValidTransition(rules.first_transition));
  assert(ValidTransition(rules.second_transition));
  assert(base.asDuration().inMinutes() +
             rules.adjustment.asDuration().inMinutes() >=
         INT16_MIN);
  assert(base.asDuration().inMinutes() +
             rules.adjustment.asDuration().inMinutes() <=
         INT16_MAX);
}

UtcOffset RecurringTimeZone::resolveOffset(WallTime instant) const {
  assert(instant >= CalendarBegin() && instant < CalendarEnd());
  const int year = DateTime(instant, timezone::UTC).year();
  const WallTime first = TransitionAt(rules_.first_transition, year,
                                      base_offset_, rules_.adjustment);
  const WallTime second = TransitionAt(rules_.second_transition, year,
                                       base_offset_, rules_.adjustment);
  const bool adjusted = instant >= first && instant < second;
  return adjusted ? UtcOffset(base_offset_.asDuration() +
                              rules_.adjustment.asDuration())
                  : base_offset_;
}

CalendarStatus RecurringTimeZone::calendarRange(TimeRange& out) const {
  const UtcOffset adjusted(base_offset_.asDuration() +
                           rules_.adjustment.asDuration());
  out = adjusted.inMinutes() < base_offset_.inMinutes()
            ? SafeCalendarRange(adjusted, base_offset_)
            : SafeCalendarRange(base_offset_, adjusted);
  return CalendarStatus::kOk;
}

CalendarStatus RecurringTimeZone::offsetSpanAt(WallTime instant,
                                               OffsetSpan& out) const {
  if (!instant.isSet()) return CalendarStatus::kInvalidArgument;
  TimeRange range{WallTime::Unset(), WallTime::Unset()};
  calendarRange(range);
  if (instant < range.begin || instant >= range.end) {
    return CalendarStatus::kOutOfRange;
  }
  if (rules_.adjustment.inMinutes() == 0) {
    out = {range, base_offset_};
    return CalendarStatus::kOk;
  }

  // Both annual changes lie in their UTC year. Three years therefore include
  // the nearest change on either side, with explicit guards at calendar edges.
  const int year = DateTime(instant, timezone::UTC).year();
  const int first_year = year == 1 ? 1 : year - 1;
  const int last_year = year == 9999 ? 9999 : year + 1;
  for (int y = first_year; y <= last_year; ++y) {
    for (const AnnualTransition& rule :
         {rules_.first_transition, rules_.second_transition}) {
      const WallTime transition =
          TransitionAt(rule, y, base_offset_, rules_.adjustment);
      if (transition <= instant && transition > range.begin) {
        range.begin = transition;
      }
      if (transition > instant && transition < range.end) {
        range.end = transition;
      }
    }
  }
  out = {range, resolveOffset(instant)};
  return CalendarStatus::kOk;
}

LocalTimeResolution RecurringTimeZone::resolveLocal(CivilDay date,
                                                    TimeOfDay time) const {
  DateTime local;
  if (!DateTime::TryFromComponents(date, time, timezone::UTC, local)) {
    return {LocalTimeStatus::kInvalidInput};
  }
  const UtcOffset offsets[] = {
      base_offset_,
      UtcOffset(base_offset_.asDuration() + rules_.adjustment.asDuration())};
  const WallTime begin = CalendarBegin();
  const WallTime end = CalendarEnd();
  LocalTimeResolution result{LocalTimeStatus::kNonexistent};
  bool out_of_range = false;
  // These are the only possible offsets, including for candidates in an
  // adjacent UTC year. Checking the actual candidate avoids assuming the local
  // year.
  for (int i = 0; i < 2; ++i) {
    if (i == 1 && offsets[1] == offsets[0]) continue;
    const WallTime candidate = local.wallTime() - offsets[i].asDuration();
    if (candidate < begin || candidate >= end) {
      out_of_range = true;
      continue;
    }
    if (resolveOffset(candidate) != offsets[i]) continue;
    if (!result.first.isSet()) {
      result.first = candidate;
      result.status = LocalTimeStatus::kUnique;
    } else {
      result.second = candidate < result.first ? result.first : candidate;
      if (candidate < result.first) result.first = candidate;
      result.status = LocalTimeStatus::kAmbiguous;
    }
  }
  if (!result.first.isSet() && out_of_range) {
    result.status = LocalTimeStatus::kOutOfRange;
  }
  return result;
}

LocalTimeResolution ResolveLocal(CivilDay date, TimeOfDay time,
                                 const TimeZone& zone) {
  if (!date.isValid() || !time.isValid()) {
    return {LocalTimeStatus::kInvalidInput};
  }
  return zone.resolveLocal(date, time);
}

DateTime ToLocal(WallTime instant, const TimeZone& zone) {
  const UtcOffset offset = zone.resolveOffset(instant);
  // Shift bounds rather than an arbitrary input timestamp to avoid overflow.
  assert(instant >= CalendarBegin() - offset.asDuration() &&
         instant < CalendarEnd() - offset.asDuration());
  return DateTime(instant, offset);
}

namespace rules {
SeasonalRules EuSummerTime() {
  return {{AnnualDateRule::LastWeekday(kMarch, kSunday), 60,
           TransitionTimeBasis::kUtc},
          {AnnualDateRule::LastWeekday(kOctober, kSunday), 60,
           TransitionTimeBasis::kUtc},
          UtcOffset(Hours(1))};
}

SeasonalRules UsDstSince2007() {
  return {{AnnualDateRule::NthWeekday(kMarch, kSunday, 2), 120,
           TransitionTimeBasis::kBaseLocal},
          {AnnualDateRule::NthWeekday(kNovember, kSunday, 1), 120,
           TransitionTimeBasis::kAdjustedLocal},
          UtcOffset(Hours(1))};
}

SeasonalRules AustralianEasternDst() {
  return {{AnnualDateRule::NthWeekday(kApril, kSunday, 1), 180,
           TransitionTimeBasis::kBaseLocal},
          {AnnualDateRule::NthWeekday(kOctober, kSunday, 1), 120,
           TransitionTimeBasis::kAdjustedLocal},
          UtcOffset(Hours(-1))};
}

SeasonalRules NewZealandDst() {
  return {{AnnualDateRule::NthWeekday(kApril, kSunday, 1), 120,
           TransitionTimeBasis::kAdjustedLocal},
          {AnnualDateRule::LastWeekday(kSeptember, kSunday), 120,
           TransitionTimeBasis::kAdjustedLocal},
          UtcOffset(Hours(-1))};
}

SeasonalRules LordHoweDst() {
  return {{AnnualDateRule::NthWeekday(kApril, kSunday, 1), 120,
           TransitionTimeBasis::kBaseLocal},
          {AnnualDateRule::NthWeekday(kOctober, kSunday, 1), 120,
           TransitionTimeBasis::kAdjustedLocal},
          UtcOffset(Minutes(-30))};
}

}  // namespace rules
}  // namespace roo_time
