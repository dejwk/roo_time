// Debounce two independent buttons while keeping each timestamp small.
//
// Mechanical contacts can bounce between pressed and released. We accept a
// change only after the raw input has stayed unchanged for 20 milliseconds.
// Each button remembers the instant of its last raw change in a four-byte
// SmallTimestamp; subtracting timestamps gives a SmallDuration in milliseconds.
// These compact values are useful when many inputs need their own timing state.
//
// The sketch replays a short input trace instead of reading physical pins, so
// it also runs in the emulator. Serial at 115200 baud prints accepted changes:
// button 1 presses at 23 ms, button 0 presses at 25 ms, then releases at 50 ms.

#include <Arduino.h>

#include "roo_time.h"

using namespace roo_time;

// Compact timestamps wrap after about 49.7 days. Their differences still work
// across rollover when the actual separation is less than 2^31 milliseconds
// (about 24.9 days). Use one clock domain and keep updates within that window.
/// Remembers raw and accepted states for one button.
class DebouncedButton {
 public:
  /// Creates an initially released button.
  DebouncedButton() = default;

  /// Returns true only when a new state has remained stable for 20 ms.
  bool update(bool pressed, SmallTimestamp now) {
    // Any raw change restarts the quiet interval, including a bounce back to
    // the previous state. Each button owns its own timestamp and state.
    if (pressed != raw_) {
      raw_ = pressed;
      changed_ = now;
    }

    // No event is needed if the accepted state already matches. Otherwise,
    // subtract timestamps and compare the elapsed SmallDuration with 20 ms.
    if (stable_ == raw_ || now - changed_ < SmallDuration::Millis(20))
      return false;

    // The input has been quiet long enough. Report this change exactly once.
    stable_ = raw_;
    return true;
  }

  /// Returns the last debounced state.
  bool pressed() const { return stable_; }

 private:
  SmallTimestamp changed_;  // Last raw transition, not an absolute date/time.
  bool raw_ = false;
  bool stable_ = false;
};

void setup() {
  Serial.begin(115200);
  Serial.printf("timestamp bytes: compact=%u full=%u\n",
                static_cast<unsigned>(sizeof(SmallTimestamp)),
                static_cast<unsigned>(sizeof(Uptime)));

  // Both inputs use the same debounce logic but keep independent state.
  DebouncedButton buttons[2];
  struct Frame {
    int millis;
    bool first;
    bool second;
  };

  // true means pressed. Button 0 bounces at 1, 3, and 5 ms; button 1 stays
  // pressed from 3 ms onward. Sampling again after 20 quiet ms accepts a
  // change.
  const Frame frames[] = {{0, false, false}, {1, true, false}, {3, false, true},
                          {5, true, true},   {23, true, true}, {25, true, true},
                          {30, false, true}, {50, false, true}};
  for (const Frame& frame : frames) {
    // Construct a repeatable instant on a synthetic timeline. In a live loop,
    // use SmallTimestamp::Now() for every input instead of these trace times.
    const SmallTimestamp now(Uptime::Start() + Millis(frame.millis));
    const bool raw[] = {frame.first, frame.second};
    for (unsigned i = 0; i < 2; ++i) {
      // update() returns true only for an accepted transition, so holding a
      // button down does not repeatedly emit a pressed event.
      if (buttons[i].update(raw[i], now)) {
        Serial.printf("%d ms: button %u %s\n", frame.millis, i,
                      buttons[i].pressed() ? "pressed" : "released");
      }
    }
  }
}

void loop() {
  // The teaching trace ran once in setup(). For real buttons, poll each GPIO
  // here and pass its pressed/released value plus SmallTimestamp::Now() to
  // update(). Keep all those readings in the same clock domain.
  Delay(Millis(100));
}
