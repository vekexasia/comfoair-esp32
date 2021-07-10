#include "comfoair.h"
#include "commands.h"
#include "../mqtt/mqtt.h"
#include "../secrets.h"
#include <esp32_can.h>


void printFrame2(CAN_FRAME *message)
{
  Serial.print(message->id, HEX);
  if (message->extended) Serial.print(" X ");
  else Serial.print(" S ");   
  Serial.print(message->length, DEC);
  for (int i = 0; i < message->length; i++) {
    Serial.print(message->data.byte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
#define subscribe(command) mqtt->subscribeTo(MQTT_PREFIX "/commands/" command, [this](char const * _1,uint8_t const * _2, int _3) { \
    Serial.print("Received: "); \
    Serial.println(command); \
    this->comfoMessage.sendCommand(command); \
  });
extern comfoair::MQTT *mqtt;
char mqttTopicMsgBuf[30];
char mqttTopicValBuf[30];
char otherBuf[30];


namespace comfoair {
  ComfoAir::ComfoAir() {}

  void ComfoAir::setup() {
    Serial.println("ESP_SMT init");
    CAN0.setCANPins(GPIO_NUM_5, GPIO_NUM_4);
    // CAN0.setDebuggingMode(true);
    CAN0.begin(50000);
    CAN0.watchFor();
  

    subscribe("ventilation_level_0");
    subscribe("ventilation_level_1");
    subscribe("ventilation_level_2");
    subscribe("ventilation_level_3");
    subscribe("boost_10_min");
    subscribe("boost_20_min");
    subscribe("boost_30_min");
    subscribe("boost_60_min");
    subscribe("boost_end");
    subscribe("auto");
    subscribe("manual");
    subscribe("bypass_activate_1h");
    subscribe("bypass_deactivate_1h");
    subscribe("bypass_auto");
    subscribe("ventilation_supply_only");
    subscribe("ventilation_supply_only_reset");
    subscribe("ventilation_extract_only");
    subscribe("ventilation_extract_only_reset");
    subscribe("temp_profile_normal");
    subscribe("temp_profile_cool");

    mqtt->subscribeTo(MQTT_PREFIX "/commands/" "ventilation_level", [this](char const * _1,uint8_t const * _2, int _3) {
      sprintf(otherBuf, "ventilation_level_%d", _2[0] - 48);
      Serial.print("Received: "); 
      Serial.println(_1); 
      Serial.println(otherBuf); 
      return this->comfoMessage.sendCommand(otherBuf);
    });
    
    mqtt->subscribeTo(MQTT_PREFIX "/commands/" "set_mode", [this](char const * _1,uint8_t const * _2, int _3) {
      if (memcmp("auto", _2, 4) == 0) {
        Serial.print("Received: "); 
        Serial.println(_1); 
        return this->comfoMessage.sendCommand("auto");
      } else {
        return this->comfoMessage.sendCommand("manual");
      }
    });
    
  }

  void ComfoAir::loop() {
    if (CAN0.read(this->canMessage)) {
      printFrame2(&this->canMessage);
      if (this->comfoMessage.decode(&this->canMessage, &this->decodedMessage)) {
        Serial.println("Decoded :)");
        Serial.print(decodedMessage.name);
        Serial.print(" - ");
        Serial.print(decodedMessage.val);
        sprintf(mqttTopicMsgBuf, "%s/%s", MQTT_PREFIX, decodedMessage.name);
        sprintf(mqttTopicValBuf, "%s", decodedMessage.val);
        mqtt->writeToTopic(mqttTopicMsgBuf, mqttTopicValBuf);
      }
    }
  }
}


