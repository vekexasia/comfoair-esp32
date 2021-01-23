#ifndef MQTT_H
#define MQTT_H

#include <inttypes.h>
#include <PubSubClient.h>
#include <map>

namespace comfoair {
  class MQTT {
    public:
      MQTT();
      void subscribeTo(char* topic, MQTT_CALLBACK_SIGNATURE);
      void setup();
      void loop();
      void writeToTopic(char *topic, char* payload);

    private:
      PubSubClient client;
      std::map<std::string, std::function<void(char*, uint8_t*, unsigned int)>> callbackMap;
      void subscribeToTopics();
      void ensureConnected();
  };
}

#endif