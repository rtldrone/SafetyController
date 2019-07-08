//
// Created by cameronearle on 7/8/2019.
//

#include <Arduino.h>
#include "XbeeSafetyRadio.h"

XbeeSafetyRadio::XbeeSafetyRadio(Stream *_serial) {
    serial = _serial;
}

uint8_t XbeeSafetyRadio::checksum(const uint8_t *buffer, size_t length) {
    uint16_t checksum = 0;
    for (unsigned int i = 0; i < length; i++) {
        checksum += buffer[i];
    }
    checksum &= 0xFFU;
    checksum = 0xFF - checksum;
    return (uint8_t) checksum;
}

void XbeeSafetyRadio::update() {
    //Variables used throughout the update process
    uint32_t time = millis();
    uint8_t byteIn = 0;
    uint16_t numBytesRead = 0;
    uint8_t computedChecksum = 0;

    if (serial->available()) {
        if (time - lastStateChangeTime > SERIAL_WATCHDOG_TIMEOUT) {
            //We're stuck in a state with data in the buffer.  Reset the state machine
            recvState = RECV_STATE_AWAITING_START_DELIMITER;
        }
    }

    switch (recvState) {
        case (RECV_STATE_AWAITING_START_DELIMITER):
            if (serial->available()) {
                byteIn = serial->read();
                if (byteIn == 0x7E) {
                    recvState = RECV_STATE_GOT_START_DELIMITER;
                    lastStateChangeTime = time;
                    DEBUG_LOG("Got Xbee starting delimiter");
                } else break;
            } else break;
        case (RECV_STATE_GOT_START_DELIMITER):
            //In this state we read the first size byte
            if (serial->available()) {
                byteIn = serial->read();
                recvPayloadSize = (byteIn << 8U); //Store the MSB of the size
                recvState = RECV_STATE_HAVE_FIRST_SIZE_BYTE;
                lastStateChangeTime = time;
                DEBUG_LOG("Got Xbee size MSB");
            } else break;
        case (RECV_STATE_HAVE_FIRST_SIZE_BYTE):
            //In this state we read the second size byte
            if (serial->available()) {
                byteIn = serial->read();
                recvPayloadSize |= byteIn; //Store the LSB of the size
                if (recvPayloadSize > RECV_BUFFER_SIZE) {
                    //Too much data, stop reading here
                    recvState = RECV_STATE_AWAITING_START_DELIMITER;
                    lastStateChangeTime = time;
                    DEBUG_LOG("Xbee payload size too large");
                    break;
                }
                recvState = RECV_STATE_HAVE_SECOND_SIZE_BYTE;
                lastStateChangeTime = time;
                DEBUG_LOG("Got Xbee size");
            } else break;
        case (RECV_STATE_HAVE_SECOND_SIZE_BYTE):
            //In this state we read the payload
            if (serial->available()) {
                numBytesRead = serial->readBytes(recvBuffer, recvPayloadSize);
                if (numBytesRead != recvPayloadSize) {
                    //We didn't read the right number of bytes
                    //This most likely indicates not enough data sent.  Stop reading here and reset
                    recvState = RECV_STATE_AWAITING_START_DELIMITER;
                    lastStateChangeTime = time;
                    DEBUG_LOG("Xbee incorrect payload read size");
                    break;
                }
                recvState = RECV_STATE_HAVE_PAYLOAD;
                lastStateChangeTime = time;
                DEBUG_LOG("Got Xbee payload");
            } else break;
        case (RECV_STATE_HAVE_PAYLOAD):
            //In this state we read the checksum from the packet and compare it with one we compute
            if (serial->available()) {
                byteIn = serial->read();
                computedChecksum = checksum(recvBuffer, recvPayloadSize);
                if (byteIn != computedChecksum) {
                    //Packet checksum mismatch, reset
                    recvState = RECV_STATE_AWAITING_START_DELIMITER;
                    lastStateChangeTime = time;
                    DEBUG_LOG("Xbee checksum mismatch");
                    break;
                }
                if (recvBuffer[FRAME_OFFSET_TO_BUFFER_INDEX(4)] != 0x83) { //0x83 is the 16 bit IO data frame type
                    //Wrong packet type, ignore it
                    recvState = RECV_STATE_AWAITING_START_DELIMITER;
                    lastStateChangeTime = time;
                    DEBUG_LOG("Xbee wrong frame type");
                    break;
                }
                DEBUG_LOG("Xbee received valid packet");
                lastValidRecvTime = time;
                analyzePacket();
                recvState = RECV_STATE_AWAITING_START_DELIMITER;
                lastStateChangeTime = time;
            } else break;
    }
}

void XbeeSafetyRadio::analyzePacket() {
    //We really only care about a single byte from this packet: the byte containing the DIO states
    //IO data starts at offset 9.  Offset 9 is the number of ADC samples (we ignore this)
    //Offset 10 contains digital input 8, as well as some ADC data (we ignore this)
    //Offset 11 contains digital inputs 0 through 7 only.  We use this offset.
    //Conveniently, the pin number also represents the number of bits to shift to read that pin
    uint8_t digitalInByte = recvBuffer[FRAME_OFFSET_TO_BUFFER_INDEX(11)];
    bool pinState = (((unsigned) digitalInByte >> ESTOP_INPUT_PIN) & 0x01U);
    if (!pinState) {
        //If the pin is low, we consider this an Estop.  Update the time accordingly
        lastEstopRecvTime = millis();
        DEBUG_LOG("Xbee received ESTOP packet!");
    }
}

bool XbeeSafetyRadio::getSafetyState() {
    uint32_t time = millis();
    if (time - lastValidRecvTime > XBEE_WATCHDOG_TIMEOUT) {
        //It's been too long since we've gotten a valid packet.
        return ESTOP;
    }
    if (time - lastEstopRecvTime < XBEE_ESTOP_TIMEOUT) {
        //The last received estop packet has not timed out yet.
        return ESTOP;
    }

    //If these two checks pass, we are considered safe to run
    return ENABLE;
}