#include <Arduino.h>

#include "roo_time.h"

using namespace roo_time;

// All timestamps come from one millisecond clock. Updates must be separated
// by less than 2^31 ms (about 24.9 days), including time spent asleep.
class DebouncedButton {
 public:
  /// Creates an initially released button.
  DebouncedButton() = default;

  /// Returns true only when a new state has remained stable for 20 ms.
  bool update(bool pressed, SmallTimestamp now) {
    if (pressed != raw_) {
      raw_ = pressed;
      changed_ = now;
    }
    if (stable_ == raw_ || now - changed_ < SmallDuration::Millis(20))
      return false;
    stable_ = raw_;
    return true;
  }

  /// Returns the last debounced state.
  bool pressed() const { return stable_; }

 private:
  SmallTimestamp changed_;
  bool raw_ = false;
  bool stable_ = false;
};

void setup() {
  Serial.begin(115200);
  Serial.printf("timestamp bytes: compact=%u full=%u\n",
                static_cast<unsigned>(sizeof(SmallTimestamp)),
                static_cast<unsigned>(sizeof(Uptime)));
  DebouncedButton buttons[2];
  struct Frame {
    int millis;
    bool first;
    bool second;
  };
  const Frame frames[] = {{0, false, false}, {1, true, false}, {3, false, true},
                          {5, true, true},   {23, true, true}, {25, true, true},
                          {30, false, true}, {50, false, true}};
  for (const Frame& frame : frames) {
    const SmallTimestamp now(Uptime::Start() + Millis(frame.millis));
    const bool raw[] = {frame.first, frame.second};
    for (unsigned i = 0; i < 2; ++i) {
      if (buttons[i].update(raw[i], now)) {
        Serial.printf("%d ms: button %u %s\n", frame.millis, i,
                      buttons[i].pressed() ? "pressed" : "released");
      }
    }
  }
}

void loop() {
  // In firmware, poll digitalRead() and update each button with
  // SmallTimestamp::Now(). The scripted trace above runs once without hardware.
  Delay(Millis(100));
}
