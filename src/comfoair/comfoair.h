#ifndef COMFOAIRClass_H
#define COMFOAIRClass_H
#include "message.h"

namespace comfoair {
  class ComfoAir {
    public:
      ComfoAir();
      void setup();
      void loop();
    private:
      CAN_FRAME canMessage;
      ComfoMessage comfoMessage;
      DecodedMessage decodedMessage;
  };
}

#endif