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

//TODO remove
uint16_t estop_counter = 0;
bool was_estopped = false;

void onWireData(int numBytes) {
    safetyWatchdog->update();
}

void setup() {
    wdt_enable(SYSTEM_WATCHDOG); //Enable the system watchdog.  This fully reboots the controller if something hangs
    Serial.begin(9600);
    Serial1.begin(9600);
    Wire.setClock(100000);
    Wire.begin(8);
    Wire.onReceive(onWireData);
    pinMode(RELAY_PIN, OUTPUT);
}

void loop() {
    bool state = ENABLE; //Assume we can enable

    //Update all safety managers
    safetyRadio->update();
    //The safety watchdog is checked in onWireData

    //Check state from all safety managers
    state &= safetyRadio->getSafetyState();
    state &= safetyWatchdog->getSafetyState();

    //TODO remove
    if (state == ESTOP) {
        if (!was_estopped) {
            estop_counter++;
            was_estopped = true;
        }
    } else {
        was_estopped = false;
    }

    Serial.println(estop_counter);

    //Set the output to the state
    digitalWrite(RELAY_PIN, state);
    wdt_reset(); //Feed the system watchdog timer
}