#ifndef RC_CONTROLLER_H
#define RC_CONTROLLER_H

#include <Arduino.h>
#include <Bluepad32.h>
#include "OTA.h"

struct ButtonEdge {
  bool last = false;

  bool rising(bool now) {
    bool r = (!last && now);
    if (r) last = now;
    return r;
  }

  bool falling(bool now) {
    bool f = (last && !now);
    if (f) last = now;
    return f;
  }
};


#endif  // RC_CONTROLLER_H