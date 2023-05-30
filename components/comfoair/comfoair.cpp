#include "comfoair.h"
#include "esphome/core/log.h"
#include <esp32_can.h>
#include "esphome.h"
#define min(a,b) ((a) < (b) ? (a): (b))

namespace esphome {
namespace comfoair {

static const char *TAG = "comfoair.component";

void Comfoair::setup(){
    CAN0.setCANPins( (gpio_num_t) this->rx_, (gpio_num_t) this->tx_);
    CAN0.begin(50000);
    CAN0.watchFor();
    register_service(&Comfoair::send_command, "send_command", {"command"});
    register_service(&Comfoair::sendHex, "send_hex", {"hexSequence"});
}


void Comfoair::send_command(std::string command) {
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

void Comfoair::sendHex(std::string hexSequenceToSend) {
    std::vector<uint8_t> bytes;
    for (unsigned int i = 0; i < hexSequenceToSend.length(); i += 2) {
        std::string byteString = hexSequenceToSend.substr(i, 2);
        uint8_t byte = (uint8_t) strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    this->sendRaw(bytes.size(), bytes.data());

}

void Comfoair::sendVector(std::vector<uint8_t> *data) {
    this->sendRaw(data->size(), data->data());
}
void Comfoair::sendRaw(uint8_t length, uint8_t *buf) {
    sequence++;
    sequence = sequence & 0x3;
    CAN_FRAME message;
    message.extended = true;
    message.rtr = 0;

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
        }
        // Send last packet
        memset(message.data.uint8, 0, 8);
        message.data.uint8[0] = dataGrams | 0x80;
        message.length = length - dataGrams * 7 + 1;
        memcpy(& message.data.uint8[1], &buf[dataGrams * 7], length - dataGrams * 7);
        CAN0.sendFrame(message);

    } else {
        CanAddress addr = CanAddress(0x11, 0x1, 0, 0, 0, 1, this->sequence);
        message.id = addr.canID();
        message.length = length;
        memcpy(message.data.uint8, buf, length);
        CAN0.sendFrame(message);

    }
}

void Comfoair::loop(){
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
        } else if (textSensors.find(PDOID) != textSensors.end()) {
            ComfoSensor<text_sensor::TextSensor, std::string (*)(uint8_t *)> el = textSensors.find(PDOID)->second;
            el.sensor->publish_state(el.conversion(vals));
        }



    }

}

void Comfoair::dump_config(){

    /**/


}

} //namespace comfoair
} //namespace esphome