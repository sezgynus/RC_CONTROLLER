#ifndef RC_CONTROLLER_H
#define RC_CONTROLLER_H

#include <Arduino.h>
#include <Bluepad32.h>

struct ButtonEdge {
    bool last = false;

    bool rising(bool now) {
        bool r = (!last && now);
        last = now;
        return r;
    }
};

#endif  // RC_CONTROLLER_H