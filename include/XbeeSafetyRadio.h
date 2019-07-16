//
// Created by cameronearle on 7/8/2019.
//

#ifndef SAFETYCONTROLLER_XBEESAFETYRADIO_H
#define SAFETYCONTROLLER_XBEESAFETYRADIO_H

#include "Constants.h"
#include <HardwareSerial.h>

//Constants
#define RECV_BUFFER_SIZE 256
#define ESTOP_INPUT_PIN 3U
#define XBEE_BAUDRATE 9600
#define XBEE_MAX_NUM_DEVICES 16

//State constants
#define RECV_STATE_AWAITING_START_DELIMITER 0
#define RECV_STATE_GOT_START_DELIMITER 1
#define RECV_STATE_HAVE_FIRST_SIZE_BYTE 2
#define RECV_STATE_HAVE_SECOND_SIZE_BYTE 3
#define RECV_STATE_HAVE_PAYLOAD 4

//Helper Macros
#define FRAME_OFFSET_TO_BUFFER_INDEX(x) x - 4

/**
 * Helper class for managing communications with the Xbee radio.
 */
class XbeeSafetyRadio {
public:
    /**
     * Creates a new XbeeRadio object
     * @param _serial The serial interface to use for communication with the Xbee
     */
    explicit XbeeSafetyRadio(Stream *_serial);

    /**
     * Computes the checksum of an Xbee API packet
     * @param buffer The buffer containing the message data
     * @param size The size of the buffer
     * @return The checksum
     */
    static uint8_t checksum(const uint8_t *buffer, size_t size);

    /**
     * Updates the Xbee serial data.  This should be run every loop.
     */
    void update();

    /**
     * Returns whether the system should be Estopped
     * @return True if the system should be e-stopped, false otherwise
     */
    bool getSafetyState();

    /**
    * Structure representing the state of an e-stop device
    */
    struct EstopDevice {
        uint8_t address[8] = {0};
        bool lastRecvState = false;
        uint32_t lastRecvTime = 0;
    };
private:
    Stream *serial;
    uint32_t lastStateChangeTime = 0;
    uint8_t recvState = RECV_STATE_AWAITING_START_DELIMITER;
    uint8_t recvBuffer[RECV_BUFFER_SIZE] = {0};

    uint16_t recvPayloadSize = 0;

    void analyzePacket();

    EstopDevice *devices[XBEE_MAX_NUM_DEVICES] = { nullptr };
    EstopDevice *findDevice(const uint8_t *address);
    int numRegisteredDevices();
    void insertOrUpdateDevice(const uint8_t *address, bool state, uint32_t time);
    bool getCombinedEstopState();
    uint32_t getLowestLastRecvTime();
};

#endif //SAFETYCONTROLLER_XBEESAFETYRADIO_H
