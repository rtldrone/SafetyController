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

XbeeSafetyRadio::EstopDevice *XbeeSafetyRadio::findDevice(const uint8_t *address) {
    for (int i = 0; i < XBEE_MAX_NUM_DEVICES; i++) {
        XbeeSafetyRadio::EstopDevice *device = devices[i];

        if (device == nullptr) {
            //We use nullptr as an "empty" element, so if we reach this it means that there isn't a device matching the
            //address
            return nullptr;
        }

        //Read each of the 8 bytes making up an address
        for (int j = 0; j < 8; j++) {
            //Compare the corresponding byte in each address
            if (device->address[j] != address[j]) {
                //We know the address isn't equal, move onto the next device
                break;
            }
            if (j == 7) {
                //We've made it to the last byte and it was equal.  This means we've found the device
                return device;
            }
        }
    }

    return nullptr; //No devices matched the criteria
}

int XbeeSafetyRadio::numRegisteredDevices() {
    int num = 0;
    for (int i = 0; i < XBEE_MAX_NUM_DEVICES; i++) {
        if (devices[i] == nullptr) {
            break;
        }
        num++;
    }
    return num;
}

void XbeeSafetyRadio::insertOrUpdateDevice(const uint8_t *address, bool state, uint32_t time) {
    EstopDevice *device = findDevice(address); //Try to find an existing device with this address

    //The device was not found
    if (device == nullptr) {
        //We need to create a new device and insert it.
        device = new EstopDevice();
        //Initialize the address
        for (int i = 0; i < 8; i++) {
            device->address[i] = address[i];
        }
        //Find the first available slot in the array
        for (int i = 0; i < XBEE_MAX_NUM_DEVICES; i++) {
            if (devices[i] == nullptr) {
                devices[i] = device; //Store the device
                break;
            }
            if (i == XBEE_MAX_NUM_DEVICES - 1) {
                //We've reached the end of the array and there was no room.  Delete the object we made to avoid
                //a memory leak (note this will only happen if we max out the number of devices)
                delete device;
                return;
            }
        }
    }

    //Update the rest of the device data
    device->lastRecvState = state;
    device->lastRecvTime = time;
}

bool XbeeSafetyRadio::getCombinedEstopState() {
    for (int i = 0; i < XBEE_MAX_NUM_DEVICES; i++) {
        if (devices[i] == nullptr) {
            break;
        }
        if (devices[i]->lastRecvState) {
            //This device is estopped, meaning the global state is E-stop
            return ESTOP;
        }
    }

    return ENABLE;
}

uint32_t XbeeSafetyRadio::getLowestLastRecvTime() {
    uint32_t lowestTime = 0xFFFFFFFFU; //Start at the max value
    for (int i = 0; i < XBEE_MAX_NUM_DEVICES; i++) {
        if (devices[i] == nullptr) {
            break; //Done searching
        }
        if (devices[i]->lastRecvTime < lowestTime) {
            lowestTime = devices[i]->lastRecvTime;
        }
    }
    if (lowestTime == 0xFFFFFFFFU) {
        return 0; //No received data.
    }
    return lowestTime;
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
                if (recvBuffer[FRAME_OFFSET_TO_BUFFER_INDEX(4)] != 0x82) { //0x82 is the 64 bit IO data frame type
                    //Wrong packet type, ignore it
                    recvState = RECV_STATE_AWAITING_START_DELIMITER;
                    lastStateChangeTime = time;
                    DEBUG_LOG("Xbee wrong frame type");
                    break;
                }
                DEBUG_LOG("Xbee received valid packet");
                analyzePacket();
                recvState = RECV_STATE_AWAITING_START_DELIMITER;
                lastStateChangeTime = time;
            } else break;
    }
}

void XbeeSafetyRadio::analyzePacket() {
    uint32_t time = millis(); //Read the time

    //We really only care about a single byte from this packet: the byte containing the DIO states
    //IO data starts at offset 9.  Offset 9 is the number of ADC samples (we ignore this)
    //Offset 19 contains digital inputs 0 through 7 only.  We use this offset.
    //Conveniently, the pin number also represents the number of bits to shift to read that pin
    uint8_t digitalInByte = recvBuffer[FRAME_OFFSET_TO_BUFFER_INDEX(19)];
    bool pinState = (((unsigned) digitalInByte >> ESTOP_INPUT_PIN) & 0x01U);

    //Now that we've received the state, we can register this data with our device array
    //The incoming device address starts at frame offset 5 and is 8 bytes long.
    uint8_t *address = recvBuffer + FRAME_OFFSET_TO_BUFFER_INDEX(5); //This points to the start of the incoming address
    //This function handles the rest for us
    insertOrUpdateDevice(address, pinState, time);
}

bool XbeeSafetyRadio::getSafetyState() {
    uint32_t time = millis();

    bool combinedState = getCombinedEstopState();
    uint32_t lastRecvTime = getLowestLastRecvTime();

    if (numRegisteredDevices() == 0) {
        //No devices have registered yet
        return ESTOP;
    }

    if (time - lastRecvTime > XBEE_WATCHDOG_TIMEOUT) {
        //It's been too long since we've gotten a valid packet.
        return ESTOP;
    }
    if (combinedState == ESTOP) {
        //The last received estop packet has not timed out yet.
        return ESTOP;
    }

    //If these two checks pass, we are considered safe to run
    return ENABLE;
}