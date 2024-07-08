#pragma once
#include <esp32_can.h>

#include "esphome/components/api/custom_api_device.h"
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate/climate_traits.h"
#include "CanAddress.h"
#include "commands.h"
#define amin(a,b) ((a) < (b) ? (a): (b))

namespace esphome {
namespace comfoair {

static const char *TAG = "comfoair.component";

template <typename T, typename CONV>
class ComfoSensor {
  public:
    T* sensor;
    float divider;
    CONV conversion;
};

class Comfoair: public Component, public climate::Climate, public esphome::api::CustomAPIDevice {

 public:
  void set_rx(int rx) { rx_ = rx; }
  void set_tx(int tx) { tx_ = tx; }
  void register_sensor(sensor::Sensor *obj, std::string key, int PDOID, int conversionType, int divider) {
    ComfoSensor<sensor::Sensor, int> *cs = new ComfoSensor<sensor::Sensor, int>();
    cs->sensor = obj;
    cs->divider = divider;
    cs->conversion = conversionType;
    sensors[PDOID] = *cs;
    this->PDOs.push_back(PDOID);
    this->PDOsMap[PDOID] = key;
  }
  void register_textSensor(text_sensor::TextSensor *obj, std::string key, int PDOID, std::string (*convLambda)(uint8_t *) ) {
    ComfoSensor<text_sensor::TextSensor, std::string (*)(uint8_t *)> *cs = new ComfoSensor<text_sensor::TextSensor, std::string (*)(uint8_t *)>();
    cs->sensor = obj;
    cs->conversion = convLambda;
    textSensors[PDOID] = *cs;
    this->PDOs.push_back(PDOID);
    this->PDOsMap[PDOID] = key;
  }
  void register_binarySensor(binary_sensor::BinarySensor *obj, std::string key, int PDOID, bool (*convLambda)(uint8_t *) ) {
    ComfoSensor<binary_sensor::BinarySensor, bool (*)(uint8_t *)> *cs = new ComfoSensor<binary_sensor::BinarySensor, bool (*)(uint8_t *)>();
    cs->sensor = obj;
    cs->conversion = convLambda;
    binarySensors[PDOID] = *cs;
    this->PDOs.push_back(PDOID);
    this->PDOsMap[PDOID] = key;
  }
  /**
    * Send a command to the ComfoAir
    * @param command The command to send
    */
  void send_command(std::string command) {
    #define CMDIF(name) if (command == #name) { \
                          this->sendVector(new std::vector<uint8_t>( CMD_ ## name )); \
                          delay(1000); \
                          this->sendVector(new std::vector<uint8_t>( CMD_ ## name )); \
                        } else
    CMDIF(ventilation_level_0)
    CMDIF(ventilation_level_1)
    CMDIF(ventilation_level_2)
    CMDIF(ventilation_level_3)
    CMDIF(boost_10_min)
    CMDIF(boost_20_min)
    CMDIF(boost_30_min)
    CMDIF(boost_60_min)
    CMDIF(boost_end)
    CMDIF(auto)
    CMDIF(manual)
    CMDIF(bypass_activate_1h)
    CMDIF(bypass_deactivate_1h)
    CMDIF(bypass_auto)
    CMDIF(ventilation_supply_only)
    CMDIF(ventilation_supply_only_reset)
    CMDIF(ventilation_extract_only)
    CMDIF(ventilation_extract_only_reset)
    CMDIF(temp_profile_normal)
    CMDIF(temp_profile_cool)
    CMDIF(temp_profile_warm)
    ;
  }
  void sendHex(std::string hexSequenceToSend) {
    std::vector<uint8_t> bytes;
    for (unsigned int i = 0; i < hexSequenceToSend.length(); i += 2) {
        std::string byteString = hexSequenceToSend.substr(i, 2);
        uint8_t byte = (uint8_t) strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    this->sendRaw(bytes.size(), bytes.data());
  }
  void sendVector(std::vector<uint8_t> *data) {
    this->sendRaw(data->size(), data->data());
  }
  void sendRaw(uint8_t length, uint8_t *buf){
     sequence++;
     sequence = sequence & 0x3;
     CAN_FRAME message;
     message.extended = true;
     message.rtr = 0;

     if (length > 8) {
         CanAddress addr = CanAddress(0x3a, 0x1, 0, 1, 0, 1, this->sequence);
         uint8_t dataGrams = length / 7;
         if (dataGrams * 7 == length)  {
             dataGrams--;
         }
         for (uint8_t i = 0; i < dataGrams; i++) {
             memset(message.data.byte, 0, 8);
             message.data.uint8[0] = i;
             message.length = amin((i*7)+7, length) - i*7 + 1;

             message.id = addr.canID();
             memcpy(& message.data.uint8[1], &buf[i * 7], message.length - 1);
             CAN0.sendFrame(message);
         }
         // Send last packet
         memset(message.data.uint8, 0, 8);
         message.data.uint8[0] = dataGrams | 0x80;
         message.length = length - dataGrams * 7 + 1;
         memcpy(& message.data.uint8[1], &buf[dataGrams * 7], length - dataGrams * 7);
         CAN0.sendFrame(message);

     } else {
         CanAddress addr = CanAddress(0x3a, 0x1, 0, 0, 0, 1, this->sequence);
         message.id = addr.canID();
         message.length = length;
         memcpy(message.data.uint8, buf, length);
         CAN0.sendFrame(message);

     }
  }

  void setup() override{
     CAN0.setCANPins( (gpio_num_t) this->rx_, (gpio_num_t) this->tx_);
     CAN0.begin(50000);
     CAN0.watchFor();
     register_service(&Comfoair::send_command, "send_command", {"command"});
     register_service(&Comfoair::sendHex, "send_hex", {"hexSequence"});
     register_service(&Comfoair::req_update_service, "req_update_service", {"PDOID"});
     register_service(&Comfoair::update_next, "update_all", {});
     this->update_next();
  }

  void update_next() {
    static size_t to_update = 0;

    if (to_update >= PDOs.size()) {
        to_update = 0;
        return;
    }

    int currentPDO = PDOs[to_update];
    // You can use currentPDO here for whatever operation you want to perform
    this->request_data(currentPDO);
    to_update++;
    ESP_LOGD(TAG, "update_next %d - iterator %d", currentPDO, to_update);
    set_timeout("update_next", 1000, [this](){this->update_next();});
  }
  void req_update_service(int pdo){
    this->request_data(pdo);
  }
  void request_data(int PDOID) {
//    CanAddress addr = CanAddress(0x3, 0x1, 0, 0, 0, 1, this->sequence);
    CAN_FRAME message;
    message.extended = true;
    message.rtr = 1;
    message.id = (PDOID << 14) + 0x40 + 0x3a;

    message.length = 0;
    CAN0.sendFrame(message);
  }

  void loop(){
    // ESP_LOGD(TAG, "loop");
    if (CAN0.read(canMessage)) {
        uint16_t PDOID = (canMessage.id & 0x01fff000) >> 14;
        uint8_t *vals = &canMessage.data.uint8[0];
        if (sensors.find(PDOID) != sensors.end()) {
            ComfoSensor<sensor::Sensor, int> el = sensors.find(PDOID)->second;
            float sensorVal;

            switch (el.conversion) {
                case 0: // UINT8
                    sensorVal = vals[0];
                    break;
                case 1: // UINT16
                    sensorVal = (vals[0] + (vals[1]<<8));
                    break;
                case 2: // UINT32
                    sensorVal = (vals[0] + (vals[1]<<8) + ((vals[2] + (vals[3]<<8))<<16));
                    break;
                case 3: // INT16
                    sensorVal = ((vals[1] < 0x80 ? (vals[0] + (vals[1] << 8)) : - ((vals[0] ^ 0xFF) + ((vals[1] ^ 0xFF) << 8) + 1)));
                    break;
            }
            if (el.divider > 1) {
                sensorVal = sensorVal / el.divider;
            }
            el.sensor->publish_state(sensorVal);
            maybeUpdateClimate(PDOID, sensorVal);
        } else if (textSensors.find(PDOID) != textSensors.end()) {
            ComfoSensor<text_sensor::TextSensor, std::string (*)(uint8_t *)> el = textSensors.find(PDOID)->second;
            el.sensor->publish_state(el.conversion(vals));
            maybeUpdateClimate(PDOID, el.conversion(vals));
        }else if (binarySensors.find(PDOID) != binarySensors.end()) {
            ComfoSensor<binary_sensor::BinarySensor, bool (*)(uint8_t *)> el = binarySensors.find(PDOID)->second;
            el.sensor->publish_state(el.conversion(vals));
        }
    }

  }

  void dump_config(){
  }

  void maybeUpdateClimate(int PDO, std::string newVal) {
    std::string sensorName = this->PDOsMap[PDO];
    if ("temp_profile" == sensorName) {
        if (newVal == "auto") {
            this->mode = climate::CLIMATE_MODE_AUTO;
        } else if (newVal == "cold") {
            this->mode = climate::CLIMATE_MODE_COOL;
        } else if (newVal == "warm") {
            this->mode = climate::CLIMATE_MODE_HEAT;
        }
        this->publish_state();
    }
  }

  void maybeUpdateClimate(int PDO, float newVal){
    std::string sensorName = this->PDOsMap[PDO];
//    ESP_LOGD(TAG, "maybeUpdateClimate %s %f", sensorName.c_str(), newVal);
    if ("fan_speed" == sensorName) {
        ESP_LOGD(TAG, "ibua %s %f", sensorName.c_str(), newVal);
        if (newVal == 0) {
            this->fan_mode = (climate::CLIMATE_FAN_OFF);
        } else if (newVal == 1) {
            this->fan_mode = (climate::CLIMATE_FAN_LOW);
        } else if (newVal == 2) {
            this->fan_mode = (climate::CLIMATE_FAN_MEDIUM);
        } else if (newVal == 3) {
            this->fan_mode = (climate::CLIMATE_FAN_HIGH);
        }
        this->publish_state();
    } else if ("extract_air_temp" == sensorName) {
       this->current_temperature = newVal;
       this->publish_state();
    } else if ("extract_air_humidity" == sensorName) {
       this->current_humidity = newVal;
       this->publish_state();
    } else if ("target_temp" == sensorName) {
       this->target_temperature = newVal;
       this->publish_state();
    }
  }

  climate::ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_supports_current_humidity(true);
    traits.set_supports_two_point_target_temperature(false);
    traits.set_supports_action(false);
    traits.set_supported_modes({climate::CLIMATE_MODE_AUTO, climate::CLIMATE_MODE_COOL, climate::CLIMATE_MODE_HEAT});
    traits.set_supported_presets({climate::CLIMATE_PRESET_HOME});
    traits.set_visual_min_temperature(10);
    traits.set_visual_max_temperature(35);
    traits.set_visual_temperature_step(1);
    traits.set_supported_fan_modes({
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH,
      climate::CLIMATE_FAN_OFF
    });
    return traits;
  }

  void control(const climate::ClimateCall &instruction) override {
    if (instruction.get_fan_mode().has_value()){
        switch (instruction.get_fan_mode().value()) {
            case climate::CLIMATE_FAN_AUTO:
            this->send_command("auto");
            break;
            case climate::CLIMATE_FAN_LOW:
            this->send_command("ventilation_level_1");
            break;
            case climate::CLIMATE_FAN_MEDIUM:
            this->send_command("ventilation_level_2");
            break;
            case climate::CLIMATE_FAN_HIGH:
            this->send_command("ventilation_level_3");
            break;
            case climate::CLIMATE_FAN_OFF:
            this->send_command("ventilation_level_0");
            break;
        }
    }
    if (instruction.get_mode().has_value()){
        switch (instruction.get_mode().value()) {
            case climate::CLIMATE_MODE_AUTO:
            this->send_command("temp_profile_normal");
            break;
            case climate::CLIMATE_MODE_COOL:
            this->send_command("temp_profile_cool");
            break;
            case climate::CLIMATE_MODE_HEAT:
            this->send_command("temp_profile_warm");
            break;
        }

    }
  }

 private:
  CAN_FRAME canMessage;

 protected:
  int rx_{-1};
  int tx_{-1};
  uint8_t sequence = 0;
  /* List of PDOs to request */
  std::vector<int> PDOs;
  /* map between pdoid and string key */
  std::map<int, std::string> PDOsMap;
  std::map<int, ComfoSensor<sensor::Sensor, int>> sensors;
  std::map<int, ComfoSensor<text_sensor::TextSensor,  std::string (*)(uint8_t *)>> textSensors;
  std::map<int, ComfoSensor<binary_sensor::BinarySensor, bool (*)(uint8_t *)>> binarySensors;

};

} //namespace comfoair
} //namespace esphome
