#pragma once

// Emulator setup for Esp32NtpTime. The sketch includes this header only during
// host runs, keeping fake hardware details separate from the time APIs it
// teaches. It supplies Wi-Fi credentials and the StartClock() hook used by the
// sketch; the on-board versions remain visible in Esp32NtpTime.ino.
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
    // The global fixture is constructed before setup(), so WiFi.begin() in
    // the sketch can find this access point through the regular Arduino API.
    FakeEsp32().setWifiEnvironment(wifi);
  }
} emulator;

const char* const kWifiSsid = "testwifi";
const char* const kWifiPassword = "pwd";
const char* const kClockReadyMessage =
    "Host clock ready (simulated sync; no NTP exchange).";

// The host shim supplies host time without an NTP exchange. Simulate
// its missing completion notification so the sketch exercises its readiness
// wait and displays the host clock. This does not test a real SNTP server.
inline void StartClock() {
  configTime(0, 0, "pool.ntp.org");
  OnTimeSync(nullptr);
}
