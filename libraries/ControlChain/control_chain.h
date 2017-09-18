#ifndef CONTROL_CHAIN_H
#define CONTROL_CHAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdint.h>
#include "config.h"


/*
****************************************************************************************************
*       MACROS
****************************************************************************************************
*/

// protocol version
#define CC_PROTOCOL_MAJOR       0
#define CC_PROTOCOL_MINOR       5

// define serial communication baud rate and frame period
#define CC_BAUD_RATE        500000      // in bps
#define CC_FRAME_PERIOD     1000        // in us

// define versions as strings
#define CC_PROTOCOL_VERSION VERSION2_STR(CC_PROTOCOL_MAJOR, CC_PROTOCOL_MINOR)
#define CC_FIRMWARE_VERSION VERSION3_STR(CC_FIRMWARE_MAJOR, CC_FIRMWARE_MINOR, CC_FIRMWARE_MICRO)


/*
****************************************************************************************************
*       CONFIGURATION
****************************************************************************************************
*/

// Do not use this file to configure the library, use instead a file named config.h
// This section is here for testing and documentation purpose

#ifdef CC_USE_INTERNAL_CONFIG

// maximum number of devices that can be created
#define CC_MAX_DEVICES          1
// maximum number of actuators that can be created per device
#define CC_MAX_ACTUATORS        1
// maximum number of assignments that can be created per actuator
#define CC_MAX_ASSIGNMENTS      1
// maximum number of options items that can be created per device
#define CC_MAX_OPTIONS_ITEMS    8

// disable string support, any string receive will be ignored
// useful for devices with few memory
#define CC_STRING_NOT_SUPPORTED

// define the size of the queue used to store the updates before send them
#define CC_UPDATES_FIFO_SIZE    10

// define firmware version
#define CC_FIRMWARE_MAJOR   0
#define CC_FIRMWARE_MINOR   0
#define CC_FIRMWARE_MICRO   0

// CC_USE_INTERNAL_CONFIG
#endif

// define new macros according configuration
// string supported
#ifndef CC_STRING_NOT_SUPPORTED
#define CC_STRING_SUPPORTED
#endif

// make sure options list is disabled if string is not supported
#if defined(CC_STRING_NOT_SUPPORTED) || !defined(CC_MAX_OPTIONS_ITEMS)
#undef CC_MAX_OPTIONS_ITEMS
#define CC_MAX_OPTIONS_ITEMS 0
#endif

// options list supported
#if CC_MAX_OPTIONS_ITEMS > 0
#define CC_OPTIONS_LIST_SUPPORTED
#endif

// fallback option for serial baud rate
#define CC_BAUD_RATE_FALLBACK   115200

// these includes have to be added after the configuration
#include "device.h"
#include "handshake.h"
#include "actuator.h"
#include "update.h"
#include "msg.h"
#include "utils.h"


/*
****************************************************************************************************
*       DATA TYPES
****************************************************************************************************
*/

typedef struct cc_data_t {
    uint8_t *data;
    uint32_t size;
} cc_data_t;

typedef struct cc_event_t {
    int id;
    void *data;
} cc_event_t;

enum {CC_EV_HANDSHAKE_FAILED, CC_EV_ASSIGNMENT, CC_EV_UNASSIGNMENT, CC_EV_UPDATE,
      CC_EV_DEVICE_DISABLED, CC_EV_MASTER_RESETED};


/*
****************************************************************************************************
*       FUNCTION PROTOTYPES
****************************************************************************************************
*/

void cc_init(void (*response_cb)(void *arg), void (*events_cb)(void *arg));
void cc_process(void);
int cc_parse(const cc_data_t *received);


/*
****************************************************************************************************
*       CONFIGURATION ERRORS
****************************************************************************************************
*/

#if !defined(CC_FIRMWARE_MAJOR) && !defined(CC_FIRMWARE_MINOR) && !defined(CC_FIRMWARE_MICRO)
#error "The macros CC_FIRMWARE_MAJOR, CC_FIRMWARE_MINOR and CC_FIRMWARE_MICRO must to be defined"
#endif


#ifdef __cplusplus
}
#endif

#endif
