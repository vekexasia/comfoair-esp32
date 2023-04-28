#pragma once


#include <inttypes.h>
#include <map>
namespace esphome {
namespace comfoair {
  class CanAddress {
    public:
      CanAddress(uint8_t srcAddr, uint8_t dstAddr, uint8_t unknownCounter, bool multimsg, bool errorOccurred, bool isRequest, uint8_t seqNr);
      uint32_t canID();
      void canIDBuf(char *buf);
    private:
      uint8_t srcAddr, dstAddr, unknownCounter, seqNr;
      bool multimsg, errorOccurred, isRequest;
  };
}
}