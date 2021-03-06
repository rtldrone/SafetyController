//
// Created by cameronearle on 7/8/2019.
//

#ifndef SAFETYCONTROLLER_CONTROLLERSAFETYWATCHDOG_H
#define SAFETYCONTROLLER_CONTROLLERSAFETYWATCHDOG_H

#include <Wire.h>

//Constants
#define I2C_ADDRESS 0x42
#define I2C_ENABLE_BYTE 0b01010101

class ControllerSafetyWatchdog {
public:
    explicit ControllerSafetyWatchdog(Stream *_twoWire);

    /**
     * Call this when an update request is received from the primary controller
     */
    void onUpdate(bool lastState);

    /**
     * Gets the current safety state
     * @return The safety state
     */
    bool getSafetyState();
private:
    Stream *twoWire;
    uint32_t lastValidRecvTime = 0;
};

#endif //SAFETYCONTROLLER_CONTROLLERSAFETYWATCHDOG_H
