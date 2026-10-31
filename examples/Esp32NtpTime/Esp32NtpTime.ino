// Read a network-synchronized ESP32 clock and display UTC and local time.
//
// roo_time's SystemClock reads wall time; configuring Wi-Fi and SNTP is the
// platform's job. This sketch connects, waits for a synchronization callback,
// then formats each reading in UTC and with a recurring summer-time offset.
// A successful clock read alone does not establish that synchronization
// happened.
//
// On a board, fill in the credentials below and open Serial at 115200 baud.
// In the emulator, emulator.h supplies a fake access point and a simulated
// readiness notification. Its clock comes from the host; no NTP packets are
// exchanged. The remaining connection, timeout, and display flow is shared.

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

// Keep clock acquisition separate from display rules. Neither zone changes
// the clock itself: both interpret the same UTC instant with different offsets.
SystemClock system_clock;
const FixedTimeZone utc(timezone::UTC);
const RecurringTimeZone local_zone(UtcOffset(Hours(1)), rules::EuSummerTime());

// SNTP calls this when it has synchronized. Record readiness; the loop will
// read and display the clock. The callback itself does not need to print.
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

  // Keep the platform clock in UTC. Apply local offsets with roo_time when
  // displaying values, rather than configuring a fixed display offset here.
  configTime(0, 0, "pool.ntp.org");
}
#endif

// A readable clock may still hold the epoch or a stale retained value.
bool WaitForSync(Duration timeout) {
  // Uptime measures this timeout even if synchronization changes wall time.
  // Adding a Duration gives a deadline; compare instants without unit
  // conversion.
  const Uptime deadline = Uptime::Now() + timeout;
  while (!time_ready) {
    if (Uptime::Now() >= deadline) return false;

    // Yield between checks so the networking task can make progress.
    Delay(Millis(10));
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  time_ready = false;

  // First establish connectivity. Use an uptime deadline to avoid waiting
  // forever if the access point is unavailable or the credentials are wrong.
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

  // Start SNTP only after connecting, then wait for its readiness notification.
  // A timeout leaves loop() able to notice a later successful synchronization.
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

  // Read once so the UTC and local displays describe exactly the same instant.
  // isSet() below detects a failed read; time_ready tracks synchronization.
  const WallTime now = system_clock.now();

  // Keep both UTC and UTC+1/+2 local fields inside supported calendar years.
  if (!now.isSet() || now < DateTime(1, 1, 1, timezone::UTC).wallTime() ||
      now >= DateTime(9999, 12, 31, timezone::UTC).wallTime()) {
    Serial.println(
        "Clock read failed or outside this example's calendar range.");
    Delay(Seconds(1));
    return;
  }

  // Resolve each display zone at this instant. The local zone chooses UTC+1
  // in winter or UTC+2 in summer. These functions return Arduino Strings and
  // preserve the numeric UTC offset in their ISO date/time output.
  Serial.print("UTC: ");
  Serial.println(FormatIsoDateTimeArduino(now, utc));
  Serial.print("Local: ");
  Serial.println(FormatIsoDateTimeArduino(now, local_zone));

  // Refresh once per second; use uptime for this wait, not the wall clock.
  Delay(Seconds(1));
}
