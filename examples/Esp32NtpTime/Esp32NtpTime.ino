#include <Arduino.h>
#include <WiFi.h>
#include <esp_sntp.h>

#include <atomic>

#include "roo_time.h"
#include "roo_time/format.h"
#include "roo_time/timezone.h"

using namespace roo_time;

// The SNTP callback runs on another task, so this shared flag is atomic.
std::atomic<bool> time_ready{false};
SystemClock system_clock;
const FixedTimeZone utc(timezone::UTC);
const RecurringTimeZone local_zone(UtcOffset(Hours(1)), rules::EuSummerTime());

void OnTimeSync(struct timeval*) { time_ready = true; }

// Keep emulator details out of the connection and display flow below.
#ifdef ROO_TESTING
#include "emulator.h"
#else
const char* const kWifiSsid = "<enter your SSID>";
const char* const kWifiPassword = "<enter your password>";
const char* const kClockReadyMessage = "SNTP synchronization confirmed.";

// Register first so a fast initial synchronization cannot be missed.
void StartClock() {
  esp_sntp_set_time_sync_notification_cb(OnTimeSync);
  configTime(0, 0, "pool.ntp.org");
}
#endif

// A readable clock may still hold the epoch or a stale retained value.
bool WaitForSync(Duration timeout) {
  const Uptime deadline = Uptime::Now() + timeout;
  while (!time_ready) {
    if (Uptime::Now() >= deadline) return false;
    Delay(Millis(10));
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  time_ready = false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(kWifiSsid, kWifiPassword);
  const Uptime deadline = Uptime::Now() + Seconds(10);
  while (WiFi.status() != WL_CONNECTED) {
    if (Uptime::Now() >= deadline) {
      Serial.println("WiFi timed out; restart to retry.");
      return;
    }
    Delay(Millis(50));
  }
  StartClock();
  if (!WaitForSync(Seconds(20))) {
    Serial.println("SNTP timed out; waiting for a later sync notification.");
    return;
  }
  Serial.println(kClockReadyMessage);
}

void loop() {
  if (!time_ready) {
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
  Serial.print("UTC: ");
  Serial.println(FormatIsoDateTimeArduino(now, utc));
  Serial.print("Local: ");
  Serial.println(FormatIsoDateTimeArduino(now, local_zone));
  Delay(Seconds(1));
}
