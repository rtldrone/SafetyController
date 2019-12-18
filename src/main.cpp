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

volatile bool lastState = ENABLE;

void onWireRequest() {
    //Update the safety watchdog with the last known enable state
    safetyWatchdog->onUpdate(lastState);
}

void setup() {
    wdt_enable(SYSTEM_WATCHDOG); //Enable the system watchdog.  This fully reboots the controller if something hangs
    Serial.begin(9600);
    Serial1.begin(9600);
    Wire.setClock(100000);
    Wire.begin(8);
    Wire.onRequest(onWireRequest);
    pinMode(RELAY_PIN, OUTPUT);
}

void loop() {
    bool state = ENABLE; //Assume we can enable

    //Update all safety managers
    safetyRadio->update();
    //The safety watchdog is checked in onWireRequest

    //Check state from all safety managers
    state &= safetyRadio->getSafetyState();
    //state &= safetyWatchdog->getSafetyState();

    //Update last state variable
    lastState = state;
    
    //Set the output to the state
    digitalWrite(RELAY_PIN, state);
    wdt_reset(); //Feed the system watchdog timer
}