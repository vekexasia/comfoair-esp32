#pragma once
#include <esp32_can.h>
#include <cmath>
#include <cstring>
#include <deque>

#include "esphome/components/api/custom_api_device.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate/climate_traits.h"
#include "CanAddress.h"
#include "commands.h"
#include "rmi.h"
#define amin(a,b) ((a) < (b) ? (a): (b))

namespace esphome {
namespace comfoair {

static const char *TAG = "comfoair.component";

class Comfoair;

// A writable RMI setting as a number or select, written through Comfoair
class ComfoairNumber : public number::Number {
 public:
  void set_parent(Comfoair *parent, size_t entry) { parent_ = parent; entry_ = entry; }
 protected:
  void control(float value) override;
  Comfoair *parent_{nullptr};
  size_t entry_{0};
};

class ComfoairSelect : public select::Select {
 public:
  void set_parent(Comfoair *parent, size_t entry) { parent_ = parent; entry_ = entry; }
 protected:
  void control(size_t index) override;
  Comfoair *parent_{nullptr};
  size_t entry_{0};
};

// The named commands, the only bytes send_command and send_hex may send
struct ComfoCommand {
  const char *name;
  std::vector<uint8_t> bytes;
};
#define COMFO_COMMAND(name) {#name, CMD_##name}
static const ComfoCommand COMFO_COMMANDS[] = {
    COMFO_COMMAND(ventilation_level_0),
    COMFO_COMMAND(ventilation_level_1),
    COMFO_COMMAND(ventilation_level_2),
    COMFO_COMMAND(ventilation_level_3),
    COMFO_COMMAND(boost_10_min),
    COMFO_COMMAND(boost_20_min),
    COMFO_COMMAND(boost_30_min),
    COMFO_COMMAND(boost_60_min),
    COMFO_COMMAND(boost_end),
    COMFO_COMMAND(auto),
    COMFO_COMMAND(manual),
    COMFO_COMMAND(bypass_activate_1h),
    COMFO_COMMAND(bypass_deactivate_1h),
    COMFO_COMMAND(bypass_auto),
    COMFO_COMMAND(ventilation_supply_only),
    COMFO_COMMAND(ventilation_supply_only_reset),
    COMFO_COMMAND(ventilation_extract_only),
    COMFO_COMMAND(ventilation_extract_only_reset),
    COMFO_COMMAND(temp_profile_normal),
    COMFO_COMMAND(temp_profile_cool),
    COMFO_COMMAND(temp_profile_warm),
};
#undef COMFO_COMMAND

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
  void register_rmiTextSensor(text_sensor::TextSensor *obj, const std::string &key) {
    RmiEntry *entry = this->add_rmi_entry_(key, RmiKind::TEXT, false);
    if (entry != nullptr) {
      entry->text_sensor = obj;
    }
  }
  void register_rmiSensor(sensor::Sensor *obj, const std::string &key) {
    RmiEntry *entry = this->add_rmi_entry_(key, RmiKind::NUMBER, false);
    if (entry != nullptr) {
      entry->sensor = obj;
    }
  }
  void register_rmiNumber(ComfoairNumber *obj, const std::string &key) {
    RmiEntry *entry = this->add_rmi_entry_(key, RmiKind::NUMBER, true);
    if (entry != nullptr) {
      entry->number = obj;
      obj->set_parent(this, this->rmi_entries_.size() - 1);
    }
  }
  void register_rmiSelect(ComfoairSelect *obj, const std::string &key) {
    RmiEntry *entry = this->add_rmi_entry_(key, RmiKind::SELECT, true);
    if (entry != nullptr) {
      entry->select = obj;
      obj->set_parent(this, this->rmi_entries_.size() - 1);
    }
  }
  /**
    * Send a command to the ComfoAir
    * @param command The command to send
    */
  void send_command(std::string command) {
    for (const auto &cmd : COMFO_COMMANDS) {
      if (command == cmd.name) {
        std::vector<uint8_t> cmd_data(cmd.bytes);
        this->sendVector(&cmd_data);
        delay(1000);
        this->sendVector(&cmd_data);
        return;
      }
    }
    ESP_LOGW(TAG, "Unknown command %s", command.c_str());
  }
  // Only the bytes of a named command are sent, anything else is refused
  void sendHex(std::string hexSequenceToSend) {
    std::vector<uint8_t> bytes;
    bytes.reserve(hexSequenceToSend.length() / 2);
    for (unsigned int i = 0; i < hexSequenceToSend.length(); i += 2) {
        std::string byteString = hexSequenceToSend.substr(i, 2);
        uint8_t byte = (uint8_t) strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    for (const auto &cmd : COMFO_COMMANDS) {
      if (bytes == cmd.bytes) {
        this->sendRaw(bytes.size(), bytes.data());
        return;
      }
    }
    ESP_LOGW(TAG, "send_hex refused, %s is not a known command", hexSequenceToSend.c_str());
  }

  void rmi_write_number(size_t index, float value) {
    this->rmi_write(index, lroundf(value * this->rmi_entries_[index].prop->divider));
  }
  // Write a setting, only if writable, supported and within the unit's range
  void rmi_write(size_t index, int32_t raw) {
    RmiEntry &entry = this->rmi_entries_[index];
    const RmiProperty *p = entry.prop;
    if (!p->writable || !entry.supported) {
      ESP_LOGW(TAG, "RMI %s: not writable on this unit", p->key);
      this->rmi_republish_(entry);
      return;
    }
    if (!entry.has_limits) {
      ESP_LOGW(TAG, "RMI %s: not written, the unit's range is not known", p->key);
      this->rmi_republish_(entry);
      return;
    }
    if (raw < entry.min || raw > entry.max) {
      ESP_LOGW(TAG, "RMI %s: %g refused, outside the unit's range %g to %g", p->key, raw / p->divider,
               entry.min / p->divider, entry.max / p->divider);
      this->rmi_republish_(entry);
      return;
    }
    ESP_LOGI(TAG, "RMI %s: writing %g", p->key, raw / p->divider);
    this->rmi_queue_.push_back({index, true, raw, false});
    this->rmi_queue_read_(index);
    this->rmi_send_next_();
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
         CanAddress addr = CanAddress(OWN_NODE_ID, COMFOAIR_NODE_ID, 0, 1, 0, 1, this->sequence);
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
         CanAddress addr = CanAddress(OWN_NODE_ID, COMFOAIR_NODE_ID, 0, 0, 0, 1, this->sequence);
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
     this->setup_ms_ = millis();
     this->rmi_read_all_();
     set_interval("rmi_refresh", RMI_REFRESH_MS, [this](){this->rmi_read_all_();});
  }

  // After WiFi, before the API server, so HA connects once the settings and their ranges are read
  float get_setup_priority() const override { return setup_priority::BEFORE_CONNECTION; }

  // Hold back the API server until the first reads are done, at most RMI_BOOT_WAIT_MS
  bool can_proceed() override {
    if (!this->rmi_waiting_ && this->rmi_queue_.empty()) {
      return true;
    }
    if (millis() - this->setup_ms_ > RMI_BOOT_WAIT_MS) {
      if (!this->rmi_boot_wait_logged_) {
        ESP_LOGW(TAG, "RMI: settings not all read after %u s, continuing", (unsigned) (RMI_BOOT_WAIT_MS / 1000));
        this->rmi_boot_wait_logged_ = true;
      }
      return true;
    }
    return false;
  }

  void update_next() {
    if (update_iterator_ >= (int)PDOs.size()) {
        update_iterator_ = 0;
        if (!boot_recheck_done_) {
            // First pass done — schedule a second pass after 30s to fix
            // boot-time CAN artifacts (e.g. fan_speed PDO returning 0 briefly).
            boot_recheck_done_ = true;
            set_timeout("update_recheck", 30000, [this](){this->update_next();});
        }
        return;
    }

    int currentPDO = PDOs[update_iterator_];
    this->request_data(currentPDO);
    update_iterator_++;
    ESP_LOGD(TAG, "update_next %d - iterator %d", currentPDO, update_iterator_);
    set_timeout("update_next", 1000, [this](){this->update_next();});
  }
  void req_update_service(float pdo){
    this->request_data((int)pdo);
  }
  void request_data(int PDOID) {
//    CanAddress addr = CanAddress(0x3, 0x1, 0, 0, 0, 1, this->sequence);
    CAN_FRAME message;
    message.extended = true;
    message.rtr = 1;
    message.id = (PDOID << 14) + 0x40 + OWN_NODE_ID;

    message.length = 0;
    CAN0.sendFrame(message);
  }

  void loop(){
    // ESP_LOGD(TAG, "loop");
    if (CAN0.read(canMessage)) {
        if ((canMessage.id & RMI_ID_MASK) == RMI_ID_MASK) {
            this->handle_rmi_frame_(canMessage);
            return;
        }
        uint16_t PDOID = (canMessage.id & 0x01fff000) >> 14;
        uint8_t *vals = &canMessage.data.uint8[0];
        
        auto sensor_it = sensors.find(PDOID);
        if (sensor_it != sensors.end()) {
            const auto& el = sensor_it->second;
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
            // PDO 65 (fan_speed) always returns 0 via RTR at boot; the real
            // value only arrives as a spontaneous TPDO when the speed changes.
            // Skip the zero until we've seen at least one non-zero reading so
            // we don't incorrectly set the climate fan_mode to OFF on restart.
            const std::string& sKey = PDOsMap.count(PDOID) ? PDOsMap[PDOID] : "";
            if (sKey == "fan_speed") {
                if (sensorVal == 0.0f && !fan_speed_initialized_) {
                    ESP_LOGD(TAG, "fan_speed: ignoring boot-time RTR zero (not yet initialized)");
                    return;
                }
                if (sensorVal > 0.0f) fan_speed_initialized_ = true;
            }
            el.sensor->publish_state(sensorVal);
            maybeUpdateClimate(PDOID, sensorVal);
        } else {
            auto text_it = textSensors.find(PDOID);
            if (text_it != textSensors.end()) {
                const auto& el = text_it->second;
                el.sensor->publish_state(el.conversion(vals));
                maybeUpdateClimate(PDOID, el.conversion(vals));
            } else {
                auto binary_it = binarySensors.find(PDOID);
                if (binary_it != binarySensors.end()) {
                    const auto& el = binary_it->second;
                    el.sensor->publish_state(el.conversion(vals));
                }
            }
        }
    }

  }

  void dump_config(){
  }

  void maybeUpdateClimate(int PDO, const std::string& newVal) {
    const std::string& sensorName = this->PDOsMap[PDO];
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
    const std::string& sensorName = this->PDOsMap[PDO];
//    ESP_LOGD(TAG, "maybeUpdateClimate %s %f", sensorName.c_str(), newVal);
    if ("fan_speed" == sensorName) {
        ESP_LOGD(TAG, "ibua %s %f", sensorName.c_str(), newVal);
        // Use integer comparison for discrete fan levels
        int fanLevel = static_cast<int>(newVal + 0.5f); // Round to nearest int
        if (fanLevel == 0) {
            this->fan_mode = (climate::CLIMATE_FAN_OFF);
        } else if (fanLevel == 1) {
            this->fan_mode = (climate::CLIMATE_FAN_LOW);
        } else if (fanLevel == 2) {
            this->fan_mode = (climate::CLIMATE_FAN_MEDIUM);
        } else if (fanLevel == 3) {
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
    traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE | climate::CLIMATE_SUPPORTS_CURRENT_HUMIDITY);
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

  static const uint32_t RMI_BOOT_WAIT_MS = 15000;
  static const uint32_t RMI_TIMEOUT_MS = 1000;
  static const uint32_t RMI_RETRY_MS = 5000;
  static const uint32_t RMI_GAP_MS = 100;        // between requests
  static const uint32_t RMI_REFRESH_MS = 600000; // read everything again
  static const uint8_t RMI_MAX_ATTEMPTS = 3;

  // A property from the table with its entity
  struct RmiEntry {
    const RmiProperty *prop;
    text_sensor::TextSensor *text_sensor = nullptr;
    sensor::Sensor *sensor = nullptr;
    ComfoairNumber *number = nullptr;
    ComfoairSelect *select = nullptr;
    bool supported = true;   // false once the unit says it doesn't know it
    bool ask_limits = true;  // false once the unit can't give range and step
    bool has_limits = false;
    int32_t min = 0, max = 0, step = 0;  // raw, as the unit sends them
  };
  struct RmiRequest {
    size_t entry;
    bool write;
    int32_t raw;  // value to write
    bool limits;  // read range and step too
    uint8_t attempts = 0;
  };
  uint32_t setup_ms_ = 0;
  bool rmi_boot_wait_logged_ = false;
  std::vector<RmiEntry> rmi_entries_;
  std::deque<RmiRequest> rmi_queue_;
  bool rmi_waiting_ = false;
  uint8_t rmi_seq_ = 0;
  uint8_t rmi_next_block_ = 0;
  std::vector<uint8_t> rmi_reply_;

  // A new entry for key, if in the table with this kind and writability
  RmiEntry *add_rmi_entry_(const std::string &key, RmiKind kind, bool writable) {
    for (const auto &prop : RMI_PROPERTIES) {
      if (key == prop.key && prop.kind == kind && prop.writable == writable) {
        this->rmi_entries_.push_back({&prop});
        return &this->rmi_entries_.back();
      }
    }
    ESP_LOGE(TAG, "RMI: %s is not in the allowed table as this kind of entity", key.c_str());
    return nullptr;
  }

  // Send the current value again, so HA shows a refused change didn't happen
  void rmi_republish_(RmiEntry &entry) {
    if (entry.number != nullptr && entry.number->has_state()) {
      entry.number->publish_state(entry.number->state);
    }
    if (entry.select != nullptr && entry.select->active_index().has_value()) {
      entry.select->publish_state(*entry.select->active_index());
    }
  }

  void rmi_queue_read_(size_t index) {
    const RmiEntry &entry = this->rmi_entries_[index];
    this->rmi_queue_.push_back({index, false, 0, entry.prop->writable && entry.ask_limits});
  }

  void rmi_read_all_() {
    for (size_t i = 0; i < this->rmi_entries_.size(); i++) {
      if (this->rmi_entries_[i].supported) {
        this->rmi_queue_read_(i);
      }
    }
    this->rmi_send_next_();
  }

  // Send the request at the front of the queue, one at a time
  void rmi_send_next_() {
    if (this->rmi_waiting_ || this->rmi_queue_.empty()) {
      return;
    }
    const RmiRequest &req = this->rmi_queue_.front();
    const RmiProperty *p = this->rmi_entries_[req.entry].prop;
    uint8_t request[4 + 4];
    uint8_t length;
    if (req.write) {
      request[0] = RMI_SET_PROPERTY;
      request[1] = p->unit;
      request[2] = p->subunit;
      request[3] = p->property;
      length = 4 + rmi_encode_int(p->type, req.raw, &request[4]);
    } else {
      request[0] = RMI_GET_PROPERTY;
      request[1] = p->unit;
      request[2] = p->subunit;
      request[3] = RMI_TYPE_VALUE | (req.limits ? RMI_TYPE_RANGE | RMI_TYPE_STEP : 0);
      request[4] = p->property;
      length = 5;
    }
    this->rmi_reply_.clear();
    this->rmi_next_block_ = 0;
    this->sendRaw(length, request);
    this->rmi_seq_ = this->sequence;
    this->rmi_waiting_ = true;
    set_timeout("rmi_timeout", RMI_TIMEOUT_MS, [this]() {
      ESP_LOGW(TAG, "RMI %s: no reply", this->rmi_entries_[this->rmi_queue_.front().entry].prop->key);
      this->rmi_done_(true);
    });
  }

  // Retry the current request, or move on to the next
  void rmi_done_(bool retry) {
    this->rmi_waiting_ = false;
    cancel_timeout("rmi_timeout");
    RmiRequest &req = this->rmi_queue_.front();
    if (retry && ++req.attempts < RMI_MAX_ATTEMPTS) {
      set_timeout("rmi_next", RMI_RETRY_MS, [this](){this->rmi_send_next_();});
      return;
    }
    this->rmi_queue_.pop_front();
    set_timeout("rmi_next", RMI_GAP_MS, [this](){this->rmi_send_next_();});
  }

  void handle_rmi_frame_(const CAN_FRAME &frame) {
    uint32_t id = frame.id;
    uint8_t src = id & 0x3F;
    uint8_t dst = (id >> 6) & 0x3F;
    bool multi = (id >> 14) & 1;
    bool error = (id >> 15) & 1;
    bool request = (id >> 16) & 1;
    uint8_t seq = (id >> 17) & 0x3;
    // Only the unit's reply to our current request
    if (!this->rmi_waiting_ || request || src != COMFOAIR_NODE_ID || dst != OWN_NODE_ID || seq != this->rmi_seq_) {
      return;
    }
    RmiRequest &req = this->rmi_queue_.front();
    RmiEntry &entry = this->rmi_entries_[req.entry];
    const char *key = entry.prop->key;
    if (multi) {
      // 7 byte blocks, each starting with its index, the last with 0x80 added
      if (frame.length < 1 || (frame.data.uint8[0] & 0x7F) != this->rmi_next_block_) {
        ESP_LOGW(TAG, "RMI %s: block out of order", key);
        this->rmi_done_(true);
        return;
      }
      this->rmi_next_block_++;
      this->rmi_reply_.insert(this->rmi_reply_.end(), &frame.data.uint8[1], &frame.data.uint8[frame.length]);
      if (!(frame.data.uint8[0] & 0x80)) {
        return;
      }
    } else {
      this->rmi_reply_.insert(this->rmi_reply_.end(), &frame.data.uint8[0], &frame.data.uint8[frame.length]);
    }
    if (error) {
      this->rmi_error_(req, entry, this->rmi_reply_.empty() ? 0 : this->rmi_reply_[0]);
      return;
    }
    if (req.write) {
      ESP_LOGI(TAG, "RMI %s: written", key);
      this->rmi_done_(false);
      return;
    }
    uint8_t expected = rmi_type_size(entry.prop->type) * (req.limits ? 4 : 1);
    if (this->rmi_reply_.size() != expected) {
      ESP_LOGW(TAG, "RMI %s: reply of %u bytes, expected %u", key, (unsigned) this->rmi_reply_.size(), (unsigned) expected);
      this->rmi_done_(true);
      return;
    }
    this->publish_rmi_(entry, req.limits);
    this->rmi_done_(false);
  }

  // Unsupported properties are handled here, so one table suits every unit
  void rmi_error_(RmiRequest &req, RmiEntry &entry, uint8_t code) {
    const char *key = entry.prop->key;
    if (req.write) {
      ESP_LOGW(TAG, "RMI %s: write refused by the unit, error %u", key, code);
      this->rmi_done_(false);
      return;
    }
    if (code == RMI_ERROR_UNKNOWN_UNIT || code == RMI_ERROR_UNKNOWN_SUBUNIT || code == RMI_ERROR_UNKNOWN_PROPERTY) {
      ESP_LOGI(TAG, "RMI %s: not supported by this unit (error %u), no longer asked", key, code);
      entry.supported = false;
      this->rmi_done_(false);
      return;
    }
    if (req.limits && (code == RMI_ERROR_NO_RANGE || code == RMI_ERROR_NOT_ACCESSIBLE)) {
      ESP_LOGI(TAG, "RMI %s: no range from this unit (error %u), reading the value only and not writing it", key, code);
      entry.ask_limits = false;
      req.limits = false;
      req.attempts = 0;
      this->rmi_waiting_ = false;
      cancel_timeout("rmi_timeout");
      set_timeout("rmi_next", RMI_GAP_MS, [this](){this->rmi_send_next_();});
      return;
    }
    ESP_LOGW(TAG, "RMI %s: unit error %u", key, code);
    this->rmi_done_(false);
  }

  // Reply is the value, then min, max and step if limits were asked for
  void publish_rmi_(RmiEntry &entry, bool limits) {
    const RmiProperty *p = entry.prop;
    const uint8_t *data = this->rmi_reply_.data();
    uint8_t size = rmi_type_size(p->type);
    int32_t raw = rmi_decode_int(p->type, data);
    float value = raw / p->divider;
    if (limits) {
      entry.has_limits = true;
      entry.min = rmi_decode_int(p->type, data + size);
      entry.max = rmi_decode_int(p->type, data + 2 * size);
      entry.step = rmi_decode_int(p->type, data + 3 * size);
      ESP_LOGD(TAG, "RMI %s: %g (range %g to %g, step %g)", p->key, value, entry.min / p->divider,
               entry.max / p->divider, entry.step / p->divider);
    }
    switch (p->kind) {
      case RmiKind::TEXT: {
        std::string text = p->text(data);
        ESP_LOGD(TAG, "RMI %s: %s", p->key, text.c_str());
        entry.text_sensor->publish_state(text);
        break;
      }
      case RmiKind::NUMBER:
        if (!limits) {
          ESP_LOGD(TAG, "RMI %s: %g", p->key, value);
        }
        if (entry.number != nullptr) {
          if (entry.has_limits) {
            // The unit's range; the step is the property's resolution, as the unit doesn't check its own step
            entry.number->traits.set_min_value(entry.min / p->divider);
            entry.number->traits.set_max_value(entry.max / p->divider);
            entry.number->traits.set_step(1 / p->divider);
          }
          entry.number->publish_state(value);
        } else {
          entry.sensor->publish_state(value);
        }
        break;
      case RmiKind::SELECT:
        if (raw >= 0 && entry.select->has_index(raw)) {
          entry.select->publish_state((size_t) raw);
        } else {
          ESP_LOGW(TAG, "RMI %s: unknown option %d", p->key, (int) raw);
        }
        break;
    }
  }

 protected:
  int rx_{-1};
  int tx_{-1};
  uint8_t sequence = 0;
  int update_iterator_ = 0;
  bool boot_recheck_done_ = false;
  bool fan_speed_initialized_ = false;
  /* List of PDOs to request */
  std::vector<int> PDOs;
  /* map between pdoid and string key */
  std::map<int, std::string> PDOsMap;
  std::map<int, ComfoSensor<sensor::Sensor, int>> sensors;
  std::map<int, ComfoSensor<text_sensor::TextSensor,  std::string (*)(uint8_t *)>> textSensors;
  std::map<int, ComfoSensor<binary_sensor::BinarySensor, bool (*)(uint8_t *)>> binarySensors;

};

inline void ComfoairNumber::control(float value) { this->parent_->rmi_write_number(this->entry_, value); }
inline void ComfoairSelect::control(size_t index) { this->parent_->rmi_write(this->entry_, index); }

} //namespace comfoair
} //namespace esphome
