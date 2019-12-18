//
// Created by cameronearle on 7/8/2019.
//

#include <Arduino.h>
#include "ControllerSafetyWatchdog.h"
#include "Constants.h"

ControllerSafetyWatchdog::ControllerSafetyWatchdog(Stream *_twoWire) {
    twoWire = _twoWire;
}

void ControllerSafetyWatchdog::onUpdate(bool lastState) {
    lastValidRecvTime = millis();

    //Send the current estop state
    uint8_t stateByte = lastState == ENABLE ? 0b00000000 : 0b00000001;
    twoWire->write(stateByte);

    DEBUG_LOG("I2C Watchdog got valid update");
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