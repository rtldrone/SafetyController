/**
 * Main source for the RTL drone project safety controller.
 */

#include <Arduino.h>
#include <Wire.h>
#include "Constants.h"

static bool system_state = ESTOP; //The current state of the system
static bool pendant_state = ESTOP; //The current state of all EStop pendants
static bool watchdog_state = ESTOP; //The current state of the primary controller I2C watchdog
static uint8_t serial_payload_buffer[100]; //Buffer to hold the (up to) 100 bytes of payload from the Xbee API
static uint8_t serial_payload_checksum; //Byte to store the checksum from serial data
static uint16_t serial_payload_length; //Integer to store the length of the serial payload
static uint8_t serial_cursor;  //Cursor to track how many bytes we've gotten
static uint32_t last_valid_packet_time; //The time at which we last received a valid packet

/**
 * Computes the checksum of an Xbee data payload.  Implemented based on Digi specifications
 * @param payload The payload array
 * @param length The length of the payload
 * @return The checksum
 */
uint8_t computePayloadChecksum(const uint8_t *payload, size_t length) {
    uint16_t checksum = 0;
    for (int i = 0; i < length; i++) {
        checksum += payload[i];
    }
    checksum &= 0xFFU;
    checksum = 0xFF - checksum;
    return (uint8_t) checksum;
}

/**
 * Processes serial data.  This function should only be called when there is valid data in the buffer.
 */
void processSerialPayload() {
    last_valid_packet_time = millis(); //Update the last valid packet time
    //TODO process serial frame
}

/**
 * Function called when we get some data from Serial1, which is the HardwareSerial object attached to the TX and RX
 * pins on the feather board.  In our setup, these pins are attached to the Xbee radio on the main board.
 */
void serialEvent1() {
    while (Serial1.available()) {
        uint8_t incoming_byte = Serial1.read(); //Read the incoming byte
        if (incoming_byte == 0x7E) { //0x7E is the Xbee message start delimiter
            serial_cursor = 0; //Reset the length cursor, ready to receive two bytes of length data
            continue; //Ignore the start delimiter (we don't want to store this in our buffer)
        }

        switch (serial_cursor) {
            case 0: //MSB of length data
                serial_payload_length = (incoming_byte << 8U); //Left shift 8 bytes, store
                break;
            case 1: //LSB of length data
                serial_payload_length |= incoming_byte; //Store the LSB
                //We now have a payload length
                if (Serial1.readBytes(serial_payload_buffer, serial_payload_length) != serial_payload_length) {
                    //The number of bytes read was not equal to the expected number of bytes.  Ignore this frame
                    return;
                }
                break;
            case 2: //We have read the payload, read in the checksum and process the serial data
                serial_payload_checksum = incoming_byte;
                if (computePayloadChecksum(serial_payload_buffer, serial_payload_length) != serial_payload_checksum) {
                    //Invalid checksum.  Ignore this frame
                    return;
                }
                processSerialPayload(); //If we're here, we have a valid payload in our buffer.  We can now process it
                break;
            default: //Unhandled state
                return;
        }
        serial_cursor++;
    }
}

void setup() {
    Serial1.begin(XBEE_SERIAL_BAUDRATE); //Open the serial port with the Xbee
    Wire.begin(); //TODO add address
    pinMode(RELAY_PIN, OUTPUT);
}

void loop() {

    digitalWrite(RELAY_PIN, system_state);
}