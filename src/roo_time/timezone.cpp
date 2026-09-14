#include "roo_time/timezone.h"

namespace roo_time {
namespace {

// Returns whether the Gregorian year is a leap year.
bool IsLeap(int year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

// Returns the length of a valid month in the Gregorian year.
int MonthDays(int month, int year) {
  const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return days[month - 1] + (month == 2 && IsLeap(year));
}

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
  if (t.basis != TransitionTimeBasis::kUtc) instant -= base.offset();
  if (t.basis == TransitionTimeBasis::kAdjustedLocal)
    instant -= adjustment.offset();
  return instant;
}

WallTime CalendarBegin() { return DateTime(1, 1, 1, timezone::UTC).wallTime(); }

WallTime CalendarEnd() {
  return DateTime(9999, 12, 31, timezone::UTC).wallTime() + Hours(24);
}

}  // namespace

bool AnnualDateRule::isValid() const {
  if (month_ < kJanuary || month_ > kDecember) return false;
  if (weekday_ < kSunday || weekday_ > kSaturday) return false;
  const int minimum_days = MonthDays(month_, 2001);
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
    const int anchor = kind_ == Kind::kLast        ? MonthDays(month_, year)
                       : kind_ == Kind::kOnOrAfter ? value_
                                                   : 1;
    const int weekday =
        DateTime(year, month_, anchor, timezone::UTC).dayOfWeek();
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
  assert(base.offset().inMinutes() + rules.adjustment.offset().inMinutes() >=
         INT16_MIN);
  assert(base.offset().inMinutes() + rules.adjustment.offset().inMinutes() <=
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
  return adjusted
             ? UtcOffset(base_offset_.offset() + rules_.adjustment.offset())
             : base_offset_;
}

DateTime ToLocal(WallTime instant, const TimeZone& zone) {
  const UtcOffset offset = zone.resolveOffset(instant);
  // Shift bounds rather than an arbitrary input timestamp to avoid overflow.
  assert(instant >= CalendarBegin() - offset.offset() &&
         instant < CalendarEnd() - offset.offset());
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
