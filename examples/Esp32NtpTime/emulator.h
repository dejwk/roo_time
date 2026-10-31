#pragma once

// Host fixture only. The production sketch keeps its SNTP setup visible.
#include <Arduino.h>

#include <memory>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/transducers/wifi/wifi.h"

// The sketch receives readiness through the same callback in both environments.
void OnTimeSync(struct timeval*);

/// Supplies an in-process access point for this sketch.
struct Emulator {
  roo_testing_transducers::wifi::Environment wifi;

  /// Attaches the access point before the sketch connects.
  Emulator() {
    auto ap = std::make_unique<roo_testing_transducers::wifi::AccessPoint>(
        roo_testing_transducers::wifi::MacAddress(1, 1, 1, 1, 1, 1),
        "testwifi");
    ap->setAuthMode(roo_testing_transducers::wifi::AUTH_WEP);
    ap->setPasswd("pwd");
    wifi.addAccessPoint(std::move(ap));
    FakeEsp32().setWifiEnvironment(wifi);
  }
} emulator;

const char* const kWifiSsid = "testwifi";
const char* const kWifiPassword = "pwd";
const char* const kClockReadyMessage =
    "Host clock ready (simulated sync; no NTP exchange).";

// The host shim reads the host clock and never exchanges NTP packets.
inline void StartClock() {
  configTime(0, 0, "pool.ntp.org");
  OnTimeSync(nullptr);
}
