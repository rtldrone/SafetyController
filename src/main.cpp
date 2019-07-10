/**
 * Main source for the RTL drone project safety controller.
 */

#include <Arduino.h>
#include <avr/wdt.h>
#include <Wire.h>
#include <ControllerSafetyWatchdog.h>
#include "Constants.h"
#include "XbeeSafetyRadio.h"

static XbeeSafetyRadio *safetyRadio = new XbeeSafetyRadio(&Serial1);
static ControllerSafetyWatchdog *safetyWatchdog = new ControllerSafetyWatchdog(&Wire);

void onWireData(int numBytes) {
    //This is empty because we read the data from I2C elsewhere, but we need to have it so the data goes in the buffer
}

void setup() {
    wdt_enable(SYSTEM_WATCHDOG); //Enable the system watchdog.  This fully reboots the controller if something hangs
#ifdef DEBUG
    Serial.begin(9600);
#endif
    Serial1.begin(9600);
    Wire.begin(8);
    Wire.onReceive(onWireData);
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
    wdt_reset(); //Feed the system watchdog timer
}