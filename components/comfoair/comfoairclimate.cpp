#include <inttypes.h>
#include "comfoairclimate.h"
namespace esphome {
namespace comfoair {
ComfoairClimate::ComfoairClimate() :
  Climate(),
  PollingComponent(500) {}
  }
  }