#include <inttypes.h>
#include "CanAddress.h"

namespace comfoair {

  CanAddress::CanAddress(uint8_t srcAddr, uint8_t dstAddr, uint8_t unknownCounter, bool multimsg, bool errorOccurred, bool isRequest, uint8_t seqNr) {
    this->srcAddr = srcAddr;
    this->dstAddr = dstAddr;
    this->unknownCounter = unknownCounter;
    this->multimsg = multimsg;
    this->errorOccurred = errorOccurred;
    this->isRequest = isRequest;
    this->seqNr = seqNr;
  }

  uint32_t CanAddress::canID() {
    uint32_t addr = 0;
    addr |= this->srcAddr << 0;
    addr |= this->dstAddr << 6;
    addr |= this->unknownCounter << 12;
    addr |= (this->multimsg ? 1: 0) << 14;
    addr |= (this->errorOccurred ? 1: 0) << 15;
    addr |= (this->isRequest ? 1 : 0) << 16;
    addr |= this->seqNr << 17;
    addr |= 0x1f << 24;
    return addr;
  }

  void CanAddress::canIDBuf(char *buf) {
    uint32_t canID = this->canID();
    buf[0] = (canID >> 24) & 0xFF;
    buf[1] = (canID >> 16) & 0xFF;
    buf[2] = (canID >> 8)  & 0xFF;
    buf[3] = canID & 0xFF;
  }
  
}