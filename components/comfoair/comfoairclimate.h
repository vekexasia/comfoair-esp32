#pragma once
#include "esphome.h"

#include "esphome/components/climate/climate.h"
#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate/climate_traits.h"
#include "comfoair.h"
namespace esphome {
namespace comfoair {


class ComfoairClimate: public climate::Climate, public PollingComponent {
 private:
   Comfoair parent;
 public:
   ComfoairClimate();

   climate::ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_supports_current_humidity(true);
    traits.set_supports_two_point_target_temperature(false);
    traits.set_supports_action(false);
    traits.set_supported_modes({climate::CLIMATE_MODE_AUTO, climate::CLIMATE_MODE_COOL, climate::CLIMATE_MODE_HEAT});
    traits.set_supported_presets({climate::CLIMATE_PRESET_HOME, climate::CLIMATE_PRESET_AWAY, climate::CLIMATE_PRESET_BOOST});
    traits.set_supported_custom_presets({"intakeonly", "exhaustonly", "balanced"});
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
  /*
    if (instruction.get_fan_mode().has_value()){
        switch ((*instruction.get_fan_mode())) {
            case climate::CLIMATE_FAN_AUTO:
            this->parent->send_command("auto");
            break;
            case climate::CLIMATE_FAN_LOW:
            this->parent->send_command("ventilation_level_1");
            break;
            case climate::CLIMATE_FAN_MEDIUM:
            this->parent->send_command("ventilation_level_2");
            break;
            case climate::CLIMATE_FAN_HIGH:
            this->parent->send_command("ventilation_level_3");
            break;
            case climate::CLIMATE_FAN_OFF:
            this->parent->send_command("ventilation_level_0");
            break;
        }
    }
    if (instruction.get_mode().has_value()){
        switch ((*instruction.get_mode())) {
            case climate::CLIMATE_MODE_AUTO:
            this->parent->send_command("temp_profile_normal");
            break;
            case climate::CLIMATE_MODE_COOL:
            this->parent->send_command("temp_profile_cool");
            break;
            case climate::CLIMATE_MODE_HEAT:
            this->parent->send_command("temp_profile_warm");
            break;
        }

    }
    */
  }
};


} //namespace comfoair
} //namespace esphome