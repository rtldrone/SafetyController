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
    explicit XbeeSafetyRadio(HardwareSerial *_serial);

    /**
     * Opens the serial port at the correct baudrate
     */
    void begin();

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

private:
    HardwareSerial *serial;
    uint32_t lastValidRecvTime = 0;
    uint32_t lastEstopRecvTime = 0;
    uint8_t recvState = RECV_STATE_AWAITING_START_DELIMITER;
    uint8_t recvBuffer[RECV_BUFFER_SIZE] = {0};

    uint16_t recvPayloadSize = 0;

    void analyzePacket();
};

#endif //SAFETYCONTROLLER_XBEESAFETYRADIO_H
