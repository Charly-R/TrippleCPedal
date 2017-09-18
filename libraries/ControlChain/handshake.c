/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdlib.h>
#include "control_chain.h"
#include "handshake.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

// size of the handshake message in bytes
#define HANDSHAKE_SIZE_BYTES    (CC_MSG_HEADER_SIZE + 2 + 7)

// size of the handshake message in microseconds
// FIXME: in the future change CC_BAUD_RATE_FALLBACK to CC_BAUD_RATE
#define HANDSHAKE_SIZE      ((10 * 1000000 * HANDSHAKE_SIZE_BYTES) / CC_BAUD_RATE_FALLBACK)

// master will wait a period of 8 devices frames to receive handshakes
#define HANDSHAKES_PERIOD   (((8 * CC_FRAME_PERIOD) / HANDSHAKE_SIZE) * HANDSHAKE_SIZE)

// macro to generate random number within a range
#define RANDOM_RANGE(min, max)  ((min) + rand() / (RAND_MAX / ((max) - (min) + 1) + 1))


/*
****************************************************************************************************
*       INTERNAL CONSTANTS
****************************************************************************************************
*/


/*
****************************************************************************************************
*       INTERNAL DATA TYPES
****************************************************************************************************
*/


/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

static cc_handshake_t g_handshake;


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

cc_handshake_t *cc_handshake_generate(uint16_t *delay_us)
{
    cc_handshake_t *handshake = &g_handshake;

    // generate random number
    uint16_t random_id = RANDOM_RANGE(0, 0xFFFF);
    handshake->random_id = random_id;

    // protocol version
    handshake->protocol.major = CC_PROTOCOL_MAJOR;
    handshake->protocol.minor = CC_PROTOCOL_MINOR;
    handshake->protocol.micro = 0;

    // firmware version
    handshake->firmware.major = CC_FIRMWARE_MAJOR;
    handshake->firmware.minor = CC_FIRMWARE_MINOR;
    handshake->firmware.micro = CC_FIRMWARE_MICRO;

    // calculate the delay based on the random id
    *delay_us = ((random_id % HANDSHAKES_PERIOD) / HANDSHAKE_SIZE) * HANDSHAKE_SIZE;

    return handshake;
}
