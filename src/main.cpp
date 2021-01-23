#include <Arduino.h>
#include <esp32_can.h>
#include "wifi/wifi.h"
#include "comfoair/comfoair.h"
#include "mqtt/mqtt.h"
#include "ota/ota.h"

comfoair::ComfoAir *comfo;
comfoair::WiFi *wifi;
comfoair::MQTT *mqtt;
comfoair::OTA *ota;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  wifi = new comfoair::WiFi();
  comfo = new comfoair::ComfoAir();
  mqtt = new comfoair::MQTT();
  ota = new comfoair::OTA();
  
  wifi->setup();
  mqtt->setup();
  comfo->setup();
  ota->setup();
  Serial.println("ESPVMC init");
  
}

void loop() {
  // put your main code here, to run repeatedly:
  wifi->loop();
  mqtt->loop();
  comfo->loop();
  ota->loop();

}