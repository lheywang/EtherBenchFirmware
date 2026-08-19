/**
 * @file    task_muxer.cpp
 * @author  l.heywang <leonard.heywang@proton.me>
 * @brief   Perform the routing of the incomming packets to the correct sources.
 *
 * @version 0.1
 * @date    2026-08-19
 *
 * @copyright Copyright (c) 2026
 *
 */

#define LOG_MODULE "ROUTER"
// ======================================================================
//                              INCLUDES
// ======================================================================
// Header
#include "task_muxer.h"

// Local libraries
#include "logger.h"

// RTOS
#include "tx_api.h"
#include "tx_block_pool.h"

// ======================================================================
//                              VARIABLES
// ======================================================================
// Router related
extern TX_BLOCK_POOL router_pool;
extern TX_QUEUE router_input;
extern TX_BLOCK_POOL router_payloads;

// Other threads
extern TX_QUEUE parser_input;
extern TX_QUEUE programmer_input;
extern TX_QUEUE sequencer_input;
extern TX_QUEUE argb_input;
extern TX_QUEUE hardware_input;
extern TX_QUEUE board_input;

// ======================================================================
//                         PRIVATE VARIABLES
// ======================================================================

// Define our output matrix
static const TX_QUEUE *outputs[]{nullptr,
                                 &parser_input,
                                 nullptr,
                                 nullptr,
                                 &sequencer_input,
                                 nullptr,
                                 &board_input,
                                 &hardware_input,
                                 &router_input,
                                 &programmer_input,
                                 &argb_input};

// ======================================================================
//                              FUNCTIONS
// ======================================================================

struct muxerInput_t *create_message(muxerDestination_t dest,
                                    muxerSource_t src,
                                    muxerMessageType_t type,
                                    uint16_t info,
                                    void *payload,
                                    uint32_t payload_size) {
    // Basic safety checks :
    if (payload_size > ROUTER_TYPICAL_PAYLOAD)
        return nullptr;

    // Now, we can allocate ourself a structure on the reserved pool :
    muxerInput_t *msg = nullptr;
    if (tx_block_allocate(&router_pool, (void **)&msg, TX_WAIT_FOREVER) != TX_SUCCESS)
        return nullptr;

    // Fill the first elements of the message
    msg->destination = dest;
    msg->source = src;
    msg->type = type;
    msg->info = info;

    // Now, allocate some memory if needed
    if ((payload_size != 0) && (payload != nullptr)) {
        if (tx_block_allocate(&router_payloads, (void **)&msg->payload, TX_WAIT_FOREVER) != TX_SUCCESS) {

            // Free the allocated message
            tx_block_release(msg);
            return nullptr;
        }

        // Copy the memory
        memcpy((void *)msg->payload, payload, payload_size);

        // Update the size
        msg->payload_size = payload_size;
    } else {

        // Ensure a default value, as the memory may not be clean when we get it.
        msg->payload = nullptr;
        msg->payload_size = 0;
    }

    // Update the number of destinations
    msg->count = __builtin_popcount(dest); // Count the number of ones.

    // Return the struct
    return msg;
}

ULONG consume_message(struct muxerInput_t *msg) {

    // First, safety checks
    if (msg == nullptr)
        return TX_SUCCESS; // Message was already freed.

    // First, decount one from the clients
    if (__sync_fetch_and_sub(&msg->count, 1) == 1) {

        // If the message was moving some data with it, clean it before.
        if (msg->payload != nullptr) {
            if (tx_block_release((void *)msg->payload) != TX_SUCCESS)
                return TX_POOL_ERROR;

            // Quick sanity safety
            msg->payload = nullptr;
            msg->payload_size = 0;
        }

        // Free the message in any cases
        if (tx_block_release((void *)msg) != TX_SUCCESS)
            return TX_POOL_ERROR;
    }

    return TX_SUCCESS;
}

struct muxerInput_t *
create_response_message(struct muxerInput_t *msg, uint16_t info, void *payload, uint32_t payload_size) {

    // Safety
    if (msg == nullptr)
        return nullptr;

    // Combine both commands :
    muxerDestination_t dest = (muxerDestination_t)msg->source;
    muxerSource_t src = (muxerSource_t)msg->destination;

    muxerInput_t *resp = create_message(dest, src, msg->type, info, payload, payload_size);

    // Mark the current input as free
    if (consume_message(msg) != TX_SUCCESS)
        return nullptr;

    // Return the response
    return resp;
}

// ======================================================================
//                              THREAD
// ======================================================================

void muxer_task(ULONG arg) {

    // Unused
    (void)arg;

    LOG("Enterring the routing task ...");

    // Constant process buffer
    muxerInput_t *cmd = nullptr;

    while (1) {

        // Fetching the next word to be pushed :
        tx_queue_receive(&router_input, &cmd, TX_WAIT_FOREVER);
        if (cmd == nullptr)
            continue;

        uint32_t dest_mask = (uint32_t)cmd->destination;

        // Loop while there IS clients to be sent. This ensure an always minimal iteration count, making the router
        // faster.
        while (dest_mask != 0) {
            int target = __builtin_ctz(dest_mask);

            TX_QUEUE *client = (TX_QUEUE *)outputs[target];

            if (client != nullptr) {
                // To do : Send to the right collector
            }

            dest_mask &= ~(1U << target);
        }
    }
}
