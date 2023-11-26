#include <Arduino.h>
#include <WiFi.h>

#include "roo_time.h"
#include "time.h"

using namespace roo_time;

const char* wifi_ssid = "<enter your SSID>";
const char* wifi_password = "<enter your password>";

const char* ntpServer = "pool.ntp.org";

// 2 hours behind UTC.
const TimeZone kLocalTz(Hours(1));

SystemClock my_clock;

void setup() {
  Serial.begin(9600);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");
  configTime(0, 0, ntpServer);
}

void loop() {
  // Obtain the absolute time.
  WallTime now = my_clock.now();

  // Convert to local time.
  DateTime dt(now, kLocalTz);

  // Print it out.
  auto t = dt.tmStruct();
  Serial.println(&t, "%A, %B %d %Y %H:%M:%S");

  delay(1000);
}
