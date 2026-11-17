#pragma once
#include <cstdint>
#include <cstdio>
#include <string>

namespace esphome {
namespace comfoair {

// CAN node ids
static const uint8_t COMFOAIR_NODE_ID = 0x01;
static const uint8_t OWN_NODE_ID = 0x3a;

// RMI frames have these top bits set, PDO broadcasts never do
static const uint32_t RMI_ID_MASK = 0x1F000000;

// RMI command and the parts of a property it can get
static const uint8_t RMI_GET_PROPERTY = 0x01;
static const uint8_t RMI_TYPE_VALUE = 0x10;
static const uint8_t RMI_TYPE_RANGE = 0x20;
static const uint8_t RMI_TYPE_STEP = 0x40;

// RMI units
static const uint8_t RMI_UNIT_NODE = 0x01;
static const uint8_t RMI_UNIT_FILTER = 0x1C;
static const uint8_t RMI_UNIT_TEMPHUMCONTROL = 0x1D;
static const uint8_t RMI_UNIT_VENTILATIONCONFIG = 0x1E;

enum class RmiType : uint8_t { UINT8, UINT16, INT16, UINT32 };

inline uint8_t rmi_type_size(RmiType type) {
  switch (type) {
    case RmiType::UINT8:
      return 1;
    case RmiType::UINT16:
    case RmiType::INT16:
      return 2;
    default:
      return 4;
  }
}

// Little endian value of the given type
inline int32_t rmi_decode_int(RmiType type, const uint8_t *data) {
  switch (type) {
    case RmiType::UINT8:
      return data[0];
    case RmiType::UINT16:
      return data[0] | (data[1] << 8);
    case RmiType::INT16:
      return (int16_t) (data[0] | (data[1] << 8));
    default:
      return (int32_t) (data[0] | (data[1] << 8) | (data[2] << 16) | ((uint32_t) data[3] << 24));
  }
}

inline std::string rmi_decode_uint8(const uint8_t *data) { return std::to_string(data[0]); }

// Version as in aiocomfoconnect version_decode(), e.g. R1.9.0
inline std::string rmi_decode_version(const uint8_t *data) {
  uint32_t v = data[0] | (data[1] << 8) | (data[2] << 16) | ((uint32_t) data[3] << 24);
  static const char stages[] = {'U', 'D', 'P', 'R'};
  char buf[24];
  snprintf(buf, sizeof(buf), "%c%u.%u.%u", stages[(v >> 30) & 3], (unsigned) ((v >> 20) & 1023),
           (unsigned) ((v >> 10) & 1023), (unsigned) (v & 1023));
  return buf;
}

// Name for an option, or its number if unknown
inline std::string rmi_option(uint8_t value, const char *const *names, uint8_t count) {
  return value < count ? names[value] : std::to_string(value);
}

inline std::string rmi_decode_control(const uint8_t *data) {
  static const char *const names[] = {"off", "auto only", "on"};
  return rmi_option(data[0], names, 3);
}

inline std::string rmi_decode_profile_mode(const uint8_t *data) {
  static const char *const names[] = {"adaptive", "fixed"};
  return rmi_option(data[0], names, 2);
}

inline std::string rmi_decode_speed(const uint8_t *data) {
  static const char *const names[] = {"slow", "medium", "fast"};
  return rmi_option(data[0], names, 3);
}

// The only RMI properties the component may read, with "get property" only.
// A property has either a text decoder (text sensor) or a divider (sensor).
struct RmiProperty {
  const char *key;
  uint8_t unit;
  uint8_t subunit;
  uint8_t property;
  RmiType type;
  bool limits;  // also read range and step
  float divider;
  std::string (*text)(const uint8_t *data);
};

static const RmiProperty RMI_PROPERTIES[] = {
    // Versions
    {"hardware_version", RMI_UNIT_NODE, 0x01, 0x05, RmiType::UINT8, false, 1, rmi_decode_uint8},
    {"firmware_version", RMI_UNIT_NODE, 0x01, 0x06, RmiType::UINT32, false, 1, rmi_decode_version},
    // Temperature, bypass and humidity settings
    {"rmot_heating_threshold", RMI_UNIT_TEMPHUMCONTROL, 0x01, 0x02, RmiType::INT16, true, 10, nullptr},
    {"rmot_cooling_threshold", RMI_UNIT_TEMPHUMCONTROL, 0x01, 0x03, RmiType::INT16, true, 10, nullptr},
    {"passive_temperature_control", RMI_UNIT_TEMPHUMCONTROL, 0x01, 0x04, RmiType::UINT8, true, 1, rmi_decode_control},
    {"humidity_comfort_control", RMI_UNIT_TEMPHUMCONTROL, 0x01, 0x06, RmiType::UINT8, true, 1, rmi_decode_control},
    {"humidity_protection", RMI_UNIT_TEMPHUMCONTROL, 0x01, 0x07, RmiType::UINT8, true, 1, rmi_decode_control},
    {"temperature_profile_mode", RMI_UNIT_TEMPHUMCONTROL, 0x01, 0x08, RmiType::UINT8, true, 1, rmi_decode_profile_mode},
    {"comfort_temperature_warm", RMI_UNIT_TEMPHUMCONTROL, 0x01, 0x0A, RmiType::INT16, true, 10, nullptr},
    {"comfort_temperature_normal", RMI_UNIT_TEMPHUMCONTROL, 0x01, 0x0B, RmiType::INT16, true, 10, nullptr},
    {"comfort_temperature_cool", RMI_UNIT_TEMPHUMCONTROL, 0x01, 0x0C, RmiType::INT16, true, 10, nullptr},
    {"passive_temperature_control_speed", RMI_UNIT_TEMPHUMCONTROL, 0x01, 0x0E, RmiType::UINT8, true, 1, rmi_decode_speed},
    // Filter
    {"filter_warning_period", RMI_UNIT_FILTER, 0x01, 0x02, RmiType::UINT16, true, 1, nullptr},
    {"filter_lifetime", RMI_UNIT_FILTER, 0x01, 0x03, RmiType::UINT16, true, 1, nullptr},
    // Commissioning, read only
    {"airflow_away", RMI_UNIT_VENTILATIONCONFIG, 0x01, 0x03, RmiType::INT16, false, 1, nullptr},
    {"airflow_low", RMI_UNIT_VENTILATIONCONFIG, 0x01, 0x04, RmiType::INT16, false, 1, nullptr},
    {"airflow_medium", RMI_UNIT_VENTILATIONCONFIG, 0x01, 0x05, RmiType::INT16, false, 1, nullptr},
    {"airflow_high", RMI_UNIT_VENTILATIONCONFIG, 0x01, 0x06, RmiType::INT16, false, 1, nullptr},
    {"bathroom_switch_on_delay", RMI_UNIT_VENTILATIONCONFIG, 0x01, 0x0B, RmiType::UINT16, false, 1, nullptr},
    {"bathroom_switch_off_delay", RMI_UNIT_VENTILATIONCONFIG, 0x01, 0x0C, RmiType::UINT8, false, 1, nullptr},
};

}  // namespace comfoair
}  // namespace esphome
