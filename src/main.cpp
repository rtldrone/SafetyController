/**
 * Main source for the RTL drone project safety controller.
 */

#include <Arduino.h>
#include <Wire.h>
#include <ControllerSafetyWatchdog.h>
#include "Constants.h"
#include "XbeeSafetyRadio.h"

static XbeeSafetyRadio *safetyRadio = new XbeeSafetyRadio(&Serial1);
static ControllerSafetyWatchdog *safetyWatchdog = new ControllerSafetyWatchdog(&Wire);

void setup() {
    safetyRadio->begin(); //Open the safety radio
    safetyWatchdog->begin();
    pinMode(RELAY_PIN, OUTPUT);
}

void loop() {
    bool state = ENABLE; //Assume we can enable

    //Update all safety managers
    safetyRadio->update();
    safetyWatchdog->update();

    //Check state from all safety managers
    state &= safetyRadio->getSafetyState();
    state &= safetyWatchdog->getSafetyState();

    //Set the output to the state
    digitalWrite(RELAY_PIN, state);
}