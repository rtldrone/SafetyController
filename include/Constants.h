//
// Created by cameronearle on 7/3/2019.
//

#ifndef SAFETYCONTROLLER_CONSTANTS_H
#define SAFETYCONTROLLER_CONSTANTS_H

//Comment out this line to disable debugging
//#define DEBUG

#ifdef DEBUG
#define DEBUG_LOG(x) Serial.print(millis()); Serial.print(':'); Serial.print(' '); Serial.print(x); Serial.println()
#else
#define DEBUG_LOG(x)
#endif

#define ESTOP 0
#define ENABLE 1

#define XBEE_SERIAL_BAUDRATE 9600 //The baudrate to use when communicating with the Xbee
#define XBEE_WATCHDOG_TIMEOUT 1000 //Time in ms before declaring an estop when no packets are received
#define XBEE_ESTOP_TIMEOUT 100 //Time in ms before we consider a received estop no longer valid
#define SERIAL_WATCHDOG_TIMEOUT 1000 //Time in ms before we start clearing the serial buffer due to a suspected bad input

#define CONTROLLER_WATCHDOG_TIMEOUT 1000 //Time in ms before declaring an estop when no updates are received from the controller

#define RELAY_PIN 13

#endif //SAFETYCONTROLLER_CONSTANTS_H
