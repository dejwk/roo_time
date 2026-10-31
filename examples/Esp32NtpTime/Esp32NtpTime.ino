#include <Arduino.h>
#include <WiFi.h>

#include <atomic>
#ifndef ROO_TESTING
#include <esp_sntp.h>
#endif
#include "roo_time.h"
#include "roo_time/format.h"
#include "roo_time/timezone.h"

using namespace roo_time;

#ifdef ROO_TESTING
const char* wifi_ssid = "testwifi";
const char* wifi_password = "pwd";
#else
const char* wifi_ssid = "<enter your SSID>";
const char* wifi_password = "<enter your password>";
#endif

std::atomic<bool> synchronized(false);
SystemClock system_clock;
const FixedTimeZone utc(timezone::UTC);
const RecurringTimeZone local_zone(UtcOffset(Hours(1)), rules::EuSummerTime());

// SNTP runs on a different task; communicate completion without a data race.
void OnTimeSync(struct timeval*) {
  synchronized.store(true, std::memory_order_release);
}

// A readable clock may still hold the epoch or a stale retained value.
bool WaitForSync(Duration timeout) {
  const Uptime deadline = Uptime::Now() + timeout;
  while (!synchronized.load(std::memory_order_acquire)) {
    if (Uptime::Now() >= deadline) return false;
    Delay(Millis(10));
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  synchronized.store(false, std::memory_order_release);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  const Uptime deadline = Uptime::Now() + Seconds(10);
  while (WiFi.status() != WL_CONNECTED) {
    if (Uptime::Now() >= deadline) {
      Serial.println("WiFi timed out; restart to retry.");
      return;
    }
    Delay(Millis(50));
  }
#ifndef ROO_TESTING
  esp_sntp_set_time_sync_notification_cb(OnTimeSync);
#endif
  configTime(0, 0, "pool.ntp.org");
#ifdef ROO_TESTING
  // The host shim reads the host clock and never exchanges NTP packets.
  Serial.println("Host clock (no NTP exchange).");
  OnTimeSync(nullptr);
#endif
  if (!WaitForSync(Seconds(20))) {
    Serial.println("SNTP timed out; waiting for a later sync notification.");
    return;
  }
#ifndef ROO_TESTING
  Serial.println("SNTP synchronization confirmed.");
#endif
}

void loop() {
  if (!synchronized.load(std::memory_order_acquire)) {
    Serial.println("Time not synchronized.");
    Delay(Seconds(1));
    return;
  }
  const WallTime now = system_clock.now();
  // Keep both UTC and UTC+1/+2 local fields inside supported calendar years.
  if (!now.isSet() || now < DateTime(1, 1, 1, timezone::UTC).wallTime() ||
      now >= DateTime(9999, 12, 31, timezone::UTC).wallTime()) {
    Serial.println(
        "Clock read failed or outside this example's calendar range.");
    Delay(Seconds(1));
    return;
  }
  char text[40];
  if (FormatIsoDateTime(now, utc, text, sizeof(text)).status == TextStatus::kOk)
    Serial.printf("UTC: %s\n", text);
  if (FormatIsoDateTime(now, local_zone, text, sizeof(text)).status ==
      TextStatus::kOk)
    Serial.printf("Local: %s\n", text);
  Delay(Seconds(1));
}

// Host-only Wi-Fi environment; the sketch above uses the regular ESP32 APIs.
#ifdef ROO_TESTING
#include <memory>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/transducers/wifi/wifi.h"
struct Emulator {
  roo_testing_transducers::wifi::Environment wifi;
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
#endif
