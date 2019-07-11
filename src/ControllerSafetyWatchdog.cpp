//
// Created by cameronearle on 7/8/2019.
//

#include <Arduino.h>
#include "ControllerSafetyWatchdog.h"
#include "Constants.h"

ControllerSafetyWatchdog::ControllerSafetyWatchdog(Stream *_twoWire) {
    twoWire = _twoWire;
}

void ControllerSafetyWatchdog::update() {
    uint8_t i = 0;
    //Using this while loop with a counter ensures that if multiple bytes come in very quickly, we'll read them all.
    //We don't want old bytes in the buffer causing us to falsely assume the system should enable, but we also don't
    //want to potentially freeze our program here if the data is coming in too fast.  Limiting ourselves to a maximum
    //of 64 bytes read per cycle solves this problem (the I2C buffer is 32 bytes)
    while (twoWire->available() && i < 64) {
        DEBUG_LOG("Got I2C Byte");
        uint8_t byteIn = twoWire->read(); //Read a byte from I2C
        if (byteIn == I2C_ENABLE_BYTE) {
            //We got a valid packet
            lastValidRecvTime = millis(); //Update the time
            DEBUG_LOG("I2C Watchdog got valid packet");
        }
        i++;
    }
}

bool ControllerSafetyWatchdog::getSafetyState() {
    uint32_t time = millis();
    //Need to check if lastValidRecvTime is less than time since interrupts can actually cause it to be bigger
    //If it is, this is considered a valid enable case.
    if (lastValidRecvTime < time && time - lastValidRecvTime > CONTROLLER_WATCHDOG_TIMEOUT) {
        //We haven't gotten an update in more time than the timeout
        return ESTOP;
    }
    return ENABLE;
}