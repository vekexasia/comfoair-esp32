#pragma once
#include <esp32_can.h>
#include "esphome.h"

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace comfoair {

template <typename T, typename CONV>
class ComfoSensor {
  public:
    T* sensor;
    float divider;
    CONV conversion;
};

class Comfoair: public Component, public esphome::api::CustomAPIDevice {
 public:
  void register_sensor(sensor::Sensor *obj, int PDOID, int conversionType, int divider) {
    ComfoSensor<sensor::Sensor, int> *cs = new ComfoSensor<sensor::Sensor, int>();
    cs->sensor = obj;
    cs->divider = divider;
    cs->conversion = conversionType;
    sensors[PDOID] = *cs;
  }
  void register_textSensor(text_sensor::TextSensor *obj, int PDOID, std::string (*convLambda)(uint8_t *) ) {
    ComfoSensor<text_sensor::TextSensor, std::string (*)(uint8_t *)> *cs = new ComfoSensor<text_sensor::TextSensor, std::string (*)(uint8_t *)>();
    cs->sensor = obj;
    cs->conversion = convLambda;
    textSensors[PDOID] = *cs;
  }
  void send_command(std::string command) ;
  void sendHex(std::string hex);
  void sendVector(std::vector<uint8_t> *data);
  void sendRaw(uint8_t length, uint8_t *buf);
  void setup() override;
  void loop() override;
  void dump_config() override;

 private:
  CAN_FRAME canMessage;

 protected:
  uint8_t sequence = 0;
  std::map<int, ComfoSensor<sensor::Sensor, int>> sensors;
  std::map<int, ComfoSensor<text_sensor::TextSensor,  std::string (*)(uint8_t *)>> textSensors;
};

} //namespace comfoair
} //namespace esphome
