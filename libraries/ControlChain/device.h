#ifndef CC_DEVICE_H
#define CC_DEVICE_H

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
#include "handshake.h"
#include "actuator.h"
#include "utils.h"


/*
****************************************************************************************************
*       MACROS
****************************************************************************************************
*/


/*
****************************************************************************************************
*       CONFIGURATION
****************************************************************************************************
*/


/*
****************************************************************************************************
*       DATA TYPES
****************************************************************************************************
*/

// device descriptor actions
enum {CC_DEVICE_DESC_REQ, CC_DEVICE_DESC_ACK};

typedef struct cc_device_t {
    cstr_t uri, label;
    cc_handshake_t *handshake;
    cc_actuator_t **actuators;
    unsigned int actuators_count;
} cc_device_t;


/*
****************************************************************************************************
*       FUNCTION PROTOTYPES
****************************************************************************************************
*/

// create a new device
cc_device_t *cc_device_new(const char *name, const char *uri);
// add actuator to device actuators list
void cc_device_actuator_add(cc_device_t *device, cc_actuator_t *actuator);
// return a device object (only one device is supported)
cc_device_t *cc_device_get(void);


/*
****************************************************************************************************
*       CONFIGURATION ERRORS
****************************************************************************************************
*/


#ifdef __cplusplus
}
#endif

#endif
