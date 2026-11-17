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

// RMI commands and units
static const uint8_t RMI_GET_PROPERTY = 0x01;
static const uint8_t RMI_TYPE_VALUE = 0x10;
static const uint8_t RMI_UNIT_NODE = 0x01;

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

// The only RMI properties the component may read, with "get property" only
struct RmiProperty {
  const char *key;
  uint8_t unit;
  uint8_t subunit;
  uint8_t property;
  uint8_t length;  // expected reply length in bytes
  std::string (*decode)(const uint8_t *data);
};

static const RmiProperty RMI_PROPERTIES[] = {
    {"hardware_version", RMI_UNIT_NODE, 0x01, 0x05, 1, rmi_decode_uint8},
    {"firmware_version", RMI_UNIT_NODE, 0x01, 0x06, 4, rmi_decode_version},
};

}  // namespace comfoair
}  // namespace esphome
