#ifndef COMFOMESSAGE_H
#define COMFOMESSAGE_H

#include <inttypes.h>
#include <PubSubClient.h>
#include <map>
#include <esp32_can.h>


namespace comfoair {
  struct DecodedMessage {
    char name[40];
    char val[15];
  };

  class ComfoMessage {
    public:
      ComfoMessage();
      void test(const char * test, const char * name, const char * expectedValue);
      bool send(uint8_t length, uint8_t * buf);
      bool send(std::vector<uint8_t> *buf);
      bool decode(CAN_FRAME *frame, DecodedMessage *message);
      bool sendCommand(char const * command);
    private:
      uint8_t sequence;
  };
}

#endif