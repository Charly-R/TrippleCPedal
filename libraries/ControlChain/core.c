/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdint.h>
#include "control_chain.h"
#include "utils.h"
#include "msg.h"
#include "handshake.h"
#include "timer.h"


/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

#define SYNC_BYTE           0xA7
#define BROADCAST_ADDRESS   0

#define I_AM_ALIVE_PERIOD   50      // in sync cycles

#define RX_BUFFER_SIZE      64 + (CC_MAX_OPTIONS_ITEMS * 20)
#define TX_BUFFER_SIZE      128


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

// communication states
enum {WAITING_SYNCING, WAITING_HANDSHAKE, WAITING_DEV_DESCRIPTOR, LISTENING_REQUESTS};

// sync message cycles definition
enum {CC_SYNC_SETUP_CYCLE, CC_SYNC_REGULAR_CYCLE, CC_SYNC_HANDSHAKE_CYCLE};

// control chain handle struct
typedef struct cc_handle_t {
    void (*response_cb)(void *arg);
    void (*events_cb)(void *arg);
    int comm_state, msg_state;
    cc_msg_t *msg_rx, *msg_tx;
    int device_id;
} cc_handle_t;


/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

static cc_handle_t g_cc_handle;


/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

static void send(cc_handle_t *handle, const cc_msg_t *msg)
{
    uint8_t *buffer = handle->msg_tx->header;

    // header
    uint32_t i = 0;
    buffer[i++] = handle->device_id;
    buffer[i++] = msg->command;
    buffer[i++] = (msg->data_size >> 0) & 0xFF;
    buffer[i++] = (msg->data_size >> 8) & 0xFF;

    // data
    if (msg->data_size > 0)
    {
        if (msg != handle->msg_tx)
        {
            for (uint32_t j = 0; j < msg->data_size; j++)
                buffer[i++] = msg->data[j];
        }
        else
        {
            i += msg->data_size;
        }
    }

    // calculate crc
    buffer[i++] = crc8(buffer, CC_MSG_HEADER_SIZE + msg->data_size);

    // send sync byte
    uint8_t sync = SYNC_BYTE;
    cc_data_t response;
    response.data = &sync;
    response.size = 1;
    handle->response_cb(&response);

    // send message
    response.data = buffer;
    response.size = i;
    handle->response_cb(&response);
}

static void raise_event(cc_handle_t *handle, int event_id, void *data)
{
    static cc_event_t event;

    if (handle->events_cb)
    {
        event.id = event_id;
        event.data = data;
        handle->events_cb(&event);
    }
}

static void parser(cc_handle_t *handle)
{
    cc_msg_t *msg_rx = handle->msg_rx;

    cc_device_t *device = cc_device_get();
    if (!device)
        return;

    // check if it's setup cycle
    int sync_cycle = msg_rx->data[0];
    if (msg_rx->command == CC_CMD_CHAIN_SYNC)
    {
        if (sync_cycle == CC_SYNC_SETUP_CYCLE)
        {
            cc_updates_clear();
            cc_assignments_clear();
            raise_event(handle, CC_EV_MASTER_RESETED, 0);
            handle->comm_state = WAITING_SYNCING;
        }
    }

    if (handle->comm_state == WAITING_SYNCING)
    {
        if (msg_rx->command == CC_CMD_CHAIN_SYNC)
        {
            // check if it's handshake sync cycle
            if (sync_cycle == CC_SYNC_HANDSHAKE_CYCLE)
            {
                // generate handshake
                uint16_t wait_us;
                device->handshake = cc_handshake_generate(&wait_us);

                // build handshake message
                cc_msg_builder(CC_CMD_HANDSHAKE, device->handshake, handle->msg_tx);

                // delay the message before send it (the delay value is based on the random id)
                // this delay should minimize the chance of handshake conflicting
                // since multiple devices can be connected at the same time
                delay_us(wait_us);

                // send message
                handle->device_id = BROADCAST_ADDRESS;
                send(handle, handle->msg_tx);

                handle->comm_state++;
            }
        }
    }
    else if (handle->comm_state == WAITING_HANDSHAKE)
    {
        static int handshake_attempts, handshake_timeout;

        if (msg_rx->command == CC_CMD_HANDSHAKE)
        {
            cc_handshake_mod_t handshake;
            cc_msg_parser(msg_rx, &handshake);

            // check whether master replied to this device
            if (device->handshake->random_id == handshake.random_id)
            {
                raise_event(handle, CC_EV_HANDSHAKE_FAILED, &handshake.status);

                // TODO: check status
                // TODO: handle channel
                handle->device_id = handshake.device_id;
                handle->comm_state++;
                handshake_attempts = 0;
                handshake_timeout = 0;
            }
            else
            {
                // if doesn't receive handshake reply in 3 attempts returns to previous state
                if (++handshake_attempts >= 3)
                {
                    handshake_attempts = 0;
                    handle->comm_state = WAITING_SYNCING;
                }
            }
        }
        else
        {
            if (++handshake_timeout >= 200)
            {
                handshake_timeout = 0;
                handle->comm_state = WAITING_SYNCING;
            }
        }
    }
    else if (handle->comm_state == WAITING_DEV_DESCRIPTOR)
    {
        static int dev_desc_timeout;

        if (msg_rx->command == CC_CMD_DEV_DESCRIPTOR)
        {
            if (msg_rx->data[0] == CC_DEVICE_DESC_REQ)
            {
                // build and send device descriptor message
                cc_msg_builder(CC_CMD_DEV_DESCRIPTOR, device, handle->msg_tx);
                send(handle, handle->msg_tx);
            }
            else if (msg_rx->data[0] == CC_DEVICE_DESC_ACK)
            {
                // device descriptor was successfully delivered
                handle->comm_state++;
                dev_desc_timeout = 0;
            }
        }
        else
        {
            if (++dev_desc_timeout >= 200)
            {
                dev_desc_timeout = 0;
                handle->comm_state = WAITING_SYNCING;
            }
        }
    }
    else if (handle->comm_state == LISTENING_REQUESTS)
    {
        if (msg_rx->command == CC_CMD_CHAIN_SYNC && sync_cycle == CC_SYNC_REGULAR_CYCLE)
        {
            // device id is used to define the communication frame
            // timer is reseted each regular sync message
            timer_set(handle->device_id * CC_FRAME_PERIOD);
        }
        else if (msg_rx->command == CC_CMD_DEV_CONTROL)
        {
            int enable;
            cc_msg_parser(msg_rx, &enable);

            // device disabled
            if (enable == 0)
            {
                int status = CC_UPDATE_REQUIRED;
                raise_event(handle, CC_EV_DEVICE_DISABLED, &status);

                // FIXME: properly disable device
                while (1);
            }
        }
        else if (msg_rx->command == CC_CMD_ASSIGNMENT)
        {
            cc_assignment_t *assignment = cc_assignment_new();
            cc_msg_parser(msg_rx, assignment);

            cc_actuator_map(assignment);
            raise_event(handle, CC_EV_ASSIGNMENT, assignment);

            cc_msg_builder(CC_CMD_ASSIGNMENT, 0, handle->msg_tx);
            send(handle, handle->msg_tx);
        }
        else if (msg_rx->command == CC_CMD_UNASSIGNMENT)
        {
            uint8_t assignment_id;
            cc_msg_parser(msg_rx, &assignment_id);

            int actuator_id = cc_assignment_delete(assignment_id);
            raise_event(handle, CC_EV_UNASSIGNMENT, &actuator_id);

            cc_msg_builder(CC_CMD_UNASSIGNMENT, 0, handle->msg_tx);
            send(handle, handle->msg_tx);
        }
    }
}

static void timer_callback(void)
{
    cc_handle_t *handle = &g_cc_handle;
    static unsigned int sync_counter;

    static uint8_t chain_sync_msg_data = CC_SYNC_REGULAR_CYCLE;
    const cc_msg_t chain_sync_msg = {
        .device_id = handle->device_id,
        .command = CC_CMD_CHAIN_SYNC,
        .data_size = sizeof (chain_sync_msg_data),
        .data = &chain_sync_msg_data
    };

    // TODO: [future/optimization] the update message shouldn't be built in the interrupt
    // handler, the time of the frame is being wasted with processing. ideally it has to be
    // cached in the main loop and the interrupt handler is only used to queue the message
    // (send command)

    if (cc_updates_count() == 0)
    {
        // the device cannot stay so long time without say hey to mod, it's very needy
        if (++sync_counter >= I_AM_ALIVE_PERIOD)
        {
            send(handle, &chain_sync_msg);
            sync_counter = 0;
        }
    }
    else
    {
        cc_msg_builder(CC_CMD_DATA_UPDATE, 0, handle->msg_tx);
        send(handle, handle->msg_tx);
        sync_counter = 0;
    }
}


/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

void cc_init(void (*response_cb)(void *arg), void (*events_cb)(void *arg))
{
    static uint8_t rx_buffer[RX_BUFFER_SIZE];
    static uint8_t tx_buffer[TX_BUFFER_SIZE];

    g_cc_handle.response_cb = response_cb;
    g_cc_handle.events_cb = events_cb;
    g_cc_handle.msg_rx = cc_msg_new(rx_buffer);
    g_cc_handle.msg_tx = cc_msg_new(tx_buffer);

    timer_init(timer_callback);
}

void cc_process(void)
{
    // process each actuator going through all assignments
    // data update messages will be queued and sent in the next frame
    cc_actuators_process(g_cc_handle.events_cb);
}

int cc_parse(const cc_data_t *received)
{
    static uint32_t total_bytes;
    static int msg_ok;

    cc_handle_t *handle = &g_cc_handle;
    cc_msg_t *msg = handle->msg_rx;

    uint16_t count = 0;
    uint32_t size = received->size;
    while (size--)
    {
        uint8_t byte = received->data[count++];
        uint16_t data_size;

        // store header bytes
        if (handle->msg_state > 0 && handle->msg_state <= CC_MSG_HEADER_SIZE)
            msg->header[handle->msg_state - 1] = byte;

        switch (handle->msg_state)
        {
            // sync
            case 0:
                if (byte == SYNC_BYTE)
                {
                    msg->data_idx = 0;
                    msg->data_size = 0;
                    handle->msg_state++;
                }
                break;

            // device id
            case 1:
                // check if it's messaging this device or a broadcast message
                if (byte == BROADCAST_ADDRESS ||
                    handle->device_id == byte ||
                    handle->device_id == BROADCAST_ADDRESS)
                {
                    msg->device_id = byte;
                    handle->msg_state++;
                }
                else
                {
                    handle->msg_state = 0;
                }
                break;

            // command
            case 2:
                msg->command = byte;
                handle->msg_state++;
                break;

            // data size LSB
            case 3:
                msg->data_size = byte;
                handle->msg_state++;
                break;

            // data size MSB
            case 4:
                data_size = byte;
                data_size <<= 8;
                data_size |= msg->data_size;
                msg->data_size = data_size;

                handle->msg_state++;

                // if no data is expected skip data retrieving step
                if (data_size == 0)
                    handle->msg_state++;
                break;

            // data
            case 5:
                msg->data[msg->data_idx++] = byte;
                if (msg->data_idx == msg->data_size)
                    handle->msg_state++;
                break;

            // crc
            case 6:
                if (crc8(msg->header, CC_MSG_HEADER_SIZE + msg->data_size) == byte)
                {
                    parser(handle);
                    msg_ok = 1;
                }

                handle->msg_state = 0;
                break;
        }

        // return error if no valid message was received after 3k bytes
        if (++total_bytes >= 3000 && !msg_ok)
        {
            handle->msg_state = 0;
            total_bytes = 0;
            return -1;
        }
    }

    return 0;
}
