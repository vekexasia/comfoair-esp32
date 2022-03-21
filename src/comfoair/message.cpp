#include "message.h"
#include <esp32_can.h>
#include "CanAddress.h"
#include "commands.h"

#define min(a,b) ((a) < (b) ? (a): (b))
#define DEBUG true
void printFrame(CAN_FRAME *message)
{
    Serial.print(message->id, HEX);
    Serial.print(" ");
    for (byte i = 0; i < message->length; i++) {
        if(message->data.byte[i] < 16) Serial.print("0");
        Serial.print(message->data.byte[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}
CAN_FRAME message;
#ifdef DEBUG
uint8_t hextob(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return 0;
}
#endif
namespace comfoair {
  void ComfoMessage::test(const char * test, const char * name, const char * expectedValue) {
    #ifdef DEBUG
    Serial.print("Testing: ");
    Serial.print(test);
    Serial.print(" - ");
    Serial.print(name);
    Serial.print(" - ");
    Serial.println(expectedValue);

    message.id = 0;    
    for (uint8_t i=0; i< 8; i++) { 
      message.id <<=4; 
      message.id += hextob(test[i]); 
    } 
    message.length = hextob(test[8]); 
    for (uint8_t i=0; i<message.length; i++) { 
      message.data.uint8[i] = (hextob(test[i*2+9]) << 4) + hextob(test[i*2+10]); 
    } 
    DecodedMessage decodeddd;  
    this->decode(&message, &decodeddd); 
    if (strcmp(decodeddd.name, name) != 0) {
      Serial.print("[ERR] Received Name: ");
      Serial.println(decodeddd.name);
    }
    if (strcmp(expectedValue, decodeddd.val) != 0) {
      Serial.print("[ERR] Received Value: ");
      Serial.println(decodeddd.val);
    }
    #endif
  }
  ComfoMessage::ComfoMessage() {
    this->sequence = 0;
    message.extended = true;
    message.rtr = 0;
    #ifdef DEBUG 
    this->test("00040041107", "away_indicator", "true");
    this->test("00040041101", "away_indicator", "false");
    this->test("00104041102", "fan_speed", "2");
    this->test("00104041100", "fan_speed", "0");
    this->test("001D4041113", "exhaust_fan_duty", "19");
    this->test("001D4041126", "exhaust_fan_duty", "38");
    this->test("001D8041110", "supply_fan_duty", "16");
    this->test("001DC0412C400", "exhaust_fan_flow", "196");
    this->test("001DC04128200", "exhaust_fan_flow", "130");
    this->test("001E00412C800", "supply_fan_flow", "200");
    this->test("001E00412C900", "supply_fan_flow", "201");
    this->test("001E40412ED07", "exhaust_fan_speed", "2029");
    this->test("001E804127F07", "supply_fan_speed", "1919");
    this->test("0020004123200", "power_consumption_current", "50");
    this->test("0020004123200", "power_consumption_current", "50");
    this->test("0035404126E03", "ah_actual", "8.78");
    this->test("0035404126F03", "ah_actual", "8.79");
    this->test("0035804125401", "ah_ytd", "340");
    this->test("0035804125301", "ah_ytd", "339");
    this->test("0035C04124415", "ah_total", "5444");
    this->test("003500412DB00", "target_temp", "21.9");
    this->test("004480412D100", "extract_air_temp", "20.9");
    this->test("0044C04123700", "exhaust_air_temp", "5.5");
    this->test("0045004120900", "outdoor_air_temp", "0.9");
    this->test("004500412FFFF", "outdoor_air_temp", "-0.1");
    this->test("004500412FFA1", "outdoor_air_temp", "-9.5");
    this->test("004500412FEB3", "outdoor_air_temp", "-33.3");
    this->test("004580412C700", "post_heater_temp_before", "19.9");
    this->test("0048804112D", "extract_air_humidity", "45");
    this->test("00488041129", "extract_air_humidity", "41");
    this->test("0048C04115F", "exhaust_air_humidity", "95");
    this->test("0048C04115E", "exhaust_air_humidity", "94");
    this->test("00490041146", "outdoor_air_humidity", "70");
    this->test("00490041143", "outdoor_air_humidity", "67");
    this->test("00490041145", "outdoor_air_humidity", "69");
    this->test("0049804111B", "supply_air_humidity", "27");
    this->test("003740412C700", "post_heater_temp_after", "19.9");
    #endif
  }

  bool ComfoMessage::sendCommand(char const * command) {
    #define CMDIF(name) if (strcmp(command, #name) == 0) { \
                          this->send(new std::vector<uint8_t>( CMD_ ## name )); \
                          delay(1000); \
                          return this->send(new std::vector<uint8_t>( CMD_ ## name )); \
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
    return false;
  }

  bool ComfoMessage::decode(CAN_FRAME *frame, DecodedMessage *message) {
    uint16_t PDOID = (frame->id & 0x00fff000) >> 14;
    uint8_t *vals = &frame->data.uint8[0];
    #define uint16 (vals[0] + (vals[1]<<8))
    #define int16 (vals[1] < 0x80 ? (vals[0] + (vals[1] << 8)) : - ((vals[0] ^ 0xFF) + ((vals[1] ^ 0xFF) << 8) + 1))
    #define uint32 (uint16 + ((vals[2] + (vals[3]<<8))<<16))
    #define LAZYSWITCH(id, key, format, transformation) case id: \
                                                  strcpy(message->name, key); \
                                                  sprintf(message->val, format, transformation); \
                                                  return true;

// For documentation on PDOID's see: https://github.com/michaelarnauts/comfoconnect/blob/master/PROTOCOL-PDO.md
    switch (PDOID) {
      LAZYSWITCH(16, "away_indicator", "%s", vals[0] == 0x07 ? "true" : "false")
      LAZYSWITCH(49, "operating_mode", "%s", vals[0] == 1 ? "limited_manual": (vals[0] == 0xff ? "auto": "unlimited_manual"))  // 01 = limited_manual, FF = auto, 05 = unlimited_manual
      LAZYSWITCH(65, "fan_speed", "%d", vals[0])
      LAZYSWITCH(66, "bypass_activation_mode", "%s", vals[0] == 0 ? "auto": (vals[0] == 1 ? "activated": "deactivated")) // 0 auto, 1 activated, 2 deactivated
      LAZYSWITCH(67, "temp_profile", "%s", vals[0] == 0 ? "auto": (vals[0] == 1 ? "cold": "warm")) // 0 auto, 1 cold, 2 warm
      LAZYSWITCH(81, "next_fan_change", "%d", uint32)
      LAZYSWITCH(82, "next_bypass_change", "%d", uint32)

      // Fans
      LAZYSWITCH(117, "exhaust_fan_duty", "%d", vals[0]) // %
      LAZYSWITCH(118, "supply_fan_duty", "%d", vals[0]) // %
      LAZYSWITCH(119, "exhaust_fan_flow", "%d", uint16) // m3/h
      LAZYSWITCH(120, "supply_fan_flow", "%d", uint16) // m3/h
      LAZYSWITCH(121, "exhaust_fan_speed", "%d", uint16) // rpm
      LAZYSWITCH(122, "supply_fan_speed", "%d", uint16) // rpm

      // Power
      LAZYSWITCH(128, "power_consumption_current", "%d", uint16) 
      LAZYSWITCH(129, "power_consumption_ytd", "%d", uint16)  // kWh
      LAZYSWITCH(130, "power_consumption_since_start", "%d", uint16)  // kWh

      LAZYSWITCH(192, "remaining_days_filter_replacement", "%d", uint16)  // kWh
      
      
      // Avoided heating section
      LAZYSWITCH(213, "ah_actual", "%.2f", uint16/ 100.0)  // watts
      LAZYSWITCH(214, "ah_ytd", "%d", uint16)  // wh
      LAZYSWITCH(215, "ah_total", "%d", uint16)  // wh
      // AVoided cooling section
      LAZYSWITCH(216, "ac_actual", "%.2f", uint16/ 100.0)  // watts
      LAZYSWITCH(217, "ac_ytd", "%d", uint16)  // wh
      LAZYSWITCH(218, "ac_total", "%d", uint16)  // wh   
      
      LAZYSWITCH(227, "bypass_state", "%d", vals[0])  // %

      // temps
      LAZYSWITCH(209, "rmot", "%.1f", uint16/ 10.0)  // C°
      LAZYSWITCH(212, "target_temp", "%.1f", uint16/ 10.0)  // C°
      LAZYSWITCH(220, "pre_heater_temp_before", "%.1f", uint16/10.0) // C°
      LAZYSWITCH(221, "post_heater_temp_after", "%.1f", uint16/10.0)  // C°
      LAZYSWITCH(274, "extract_air_temp", "%.1f", int16 /10.0)  // C°   
      LAZYSWITCH(275, "exhaust_air_temp", "%.1f", int16 /10.0)  // C°   
      LAZYSWITCH(276, "outdoor_air_temp", "%.1f", int16 /10.0)  // C°   
      LAZYSWITCH(277, "pre_heater_temp_after", "%.1f", int16 /10.0)  // C°
      LAZYSWITCH(278, "post_heater_temp_before", "%.1f", int16 /10.0)  // C°   
      // Humidity
      LAZYSWITCH(290, "extract_air_humidity", "%d", vals[0])  // %
      LAZYSWITCH(291, "exhaust_air_humidity", "%d", vals[0])  // %   
      LAZYSWITCH(292, "outdoor_air_humidity", "%d", vals[0])  // %   
      LAZYSWITCH(293, "pre_heater_humidity_after", "%d", vals[0]) // %
      LAZYSWITCH(294, "supply_air_humidity", "%d", vals[0])  // %   
      default: 
        return false;
    }
  }
  
  bool ComfoMessage::send(std::vector<uint8_t> *buf) {
    return this->send(buf->size(), buf->data());
  }

  bool ComfoMessage::send(uint8_t length, uint8_t *buf) {
    this->sequence++;
    this->sequence = this->sequence & 0x3;
    message.extended = true;
    if (length > 8) {
      CanAddress addr = CanAddress(0x11, 0x1, 0, 1, 0, 1, this->sequence);
      uint8_t dataGrams = length / 7;
      if (dataGrams * 7 == length)  {
        dataGrams--;
      }

      for (uint8_t i = 0; i < dataGrams; i++) {
        memset(message.data.byte, 0, 8);
        message.data.uint8[0] = i;
        message.length = min((i*7)+7, length) - i*7 + 1;
        message.id = addr.canID();
        memcpy(& message.data.uint8[1], &buf[i * 7], message.length - 1);
        CAN0.sendFrame(message);
        printFrame(&message);
      }


      // Send last packet
      memset(message.data.uint8, 0, 8);
      message.data.uint8[0] = dataGrams | 0x80;
      message.length = length - dataGrams * 7 + 1;
      memcpy(& message.data.uint8[1], &buf[dataGrams * 7], length - dataGrams * 7);
      CAN0.sendFrame(message);
      printFrame(&message);

    } else {
      CanAddress addr = CanAddress(0x11, 0x1, 0, 0, 0, 1, this->sequence);
      message.id = addr.canID();
      message.length = length;
      memcpy(message.data.uint8, buf, length);
      CAN0.sendFrame(message);
      printFrame(&message);

    }
    return true;
  }

} 