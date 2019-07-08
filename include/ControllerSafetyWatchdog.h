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
    ControllerSafetyWatchdog(TwoWire *_twoWire);

    /**
     * Opens the I2C connection with the correct address
     */
    void begin();

    /**
     * Updates the watchdog I2C data.  This should be run every loop
     */
    void update();

    /**
     * Gets the current safety state
     * @return The safety state
     */
    bool getSafetyState();
private:
    TwoWire *twoWire;
    uint32_t lastValidRecvTime;
};

#endif //SAFETYCONTROLLER_CONTROLLERSAFETYWATCHDOG_H
