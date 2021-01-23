#include "../secrets.h"
#include <WiFi.h>
#include "wifi.h"

namespace comfoair {
  void WiFi::setup() {
      Serial.println();
      Serial.print("Connecting to ");
      Serial.println(WIFI_SSID);
      ::WiFi.begin(WIFI_SSID, WIFI_PASS);
      ::WiFi.setAutoReconnect(true);
      while (::WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
      }
      Serial.println("");
      Serial.println("WiFi Connected. IP: ");
      Serial.println(::WiFi.localIP());
  }

  void WiFi::loop() {
      //  NOthing to do!
  }
} 