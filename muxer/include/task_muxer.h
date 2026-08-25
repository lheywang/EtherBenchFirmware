/**
 * @file    task_muxer.h
 * @author  l.heywang <leonard.heywang@proton.me>
 * @brief   Header to define the bases of the main message router on the system.
 *          Handle the routing of messages from and to right components available.
 *
 * @version 0.1
 * @date    2026-08-19
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ======================================================================
//                              INCLUDES
// ======================================================================
// Local library
#include "app_threadx.h"
#include "logger.h"

// RTOS
#include "tx_api.h"

// STD
#include <stdint.h>

// ======================================================================
//                              COMMON
// ======================================================================
// Returns codes. These are aliases to the standard ThreadX one to not
// add another layer of valid returns codes.
#define CMD_OK                 TX_SUCCESS
#define CMD_REFUSED            TX_ERROR
#define CMD_INVALID_PAYLOAD    TX_PTR_ERROR
#define CMD_RESSOURCE_RESERVED TX_PRIORITY_ERROR

// ======================================================================
//                              TYPEDEFS
// ======================================================================
/**
 * @brief Define the source of the message. This value must be
 *        hardcoded for any task, to ensure it could be traced
 *        back.
 *
 */
typedef enum {
    loggerSource = (1 << 1),
    parserSource = (1 << 2),
    networkSource = (1 << 3),
    usbSource = (1 << 4),
    sequencerSource = (1 << 5),
    fileSource = (1 << 6),
    boardSource = (1 << 7),
    hardwareSource = (1 << 8),
    routerSource = (1 << 9),
    programmerSource = (1 << 10),
    argbSource = (1 << 11),
} muxerSource_t;

/**
 * @brief Define the destination of the message. These values can
 *        be masked if multiple targets are aimed.
 */
typedef enum {
    loggerDestination = (1 << 1),
    parserDestination = (1 << 2),
    networkDestination = (1 << 3),
    usbDestination = (1 << 4),
    sequencerDestination = (1 << 5),
    fileDestination = (1 << 6),
    boardDestination = (1 << 7),
    hardwareDestination = (1 << 8),
    routerDestination = (1 << 9),
    programmerDestination = (1 << 10),
    argbDestination = (1 << 11),
    boardcastDestination = 0xFFFF,
} muxerDestination_t;

/**
 * @brief Define the type of the message. The muxer does
 *        not care about this field, it's only for the user.
 *
 */
typedef enum { muxerCommand, muxerBuffer, muxerRequest } muxerMessageType_t;

// ======================================================================
//                              STRUCTS
// ======================================================================
/**
 * @brief Define the structure that is sent over the internal "network"
 *        to target the right task. It's quite simple : Fill the fields
 *        right values using an helper function, and that's done.
 *
 * @warning     This function, as any in the project that perform memory
 *              transfers are subject to a tricky point : The garbage
 *              collection. This is actually handled by the inbuilt
 *              commands.
 *
 */
struct muxerInput_t {

    // Router condition fields
    muxerSource_t source; /** The source of the message. Used as destination for the response. */
    muxerDestination_t destination /** The destination of the forward message. */;

    // User metadatas
    muxerMessageType_t type /** Message type, to make easier the sorting routine. */;
    uint16_t info /** A 16 bit field, free for the user. Value is not used by the network in any cases. */;
    uint16_t count /** A 16 bits to trigger the memory free. Set to the number of destinations. Atomically decremented
                  after each process. */
        ;

    // Optionnal payload
    void *payload /** A pointer to the payload value. Can be NULL. */;
    uint32_t payload_size /** The size of the payload. */;
};

// ======================================================================
//                              DEFINES
// ======================================================================
// Router input config
#define ROUTER_COMMAND_FIFO_DEPTH 64

// Size of the structure
#define ROUTER_COMMAND_SIZE    sizeof(struct muxerInput_t)
#define ROUTER_TYPICAL_PAYLOAD 512

// Total size to be allocated.
#define ROUTER_COMMAND_FIFO_SIZE (ROUTER_COMMAND_FIFO_DEPTH * 2 + sizeof(void *) * ROUTER_COMMAND_SIZE)
#define ROUTER_PAYLOAD_POOL_SIZE (ROUTER_COMMAND_FIFO_DEPTH * ROUTER_TYPICAL_PAYLOAD)

// ======================================================================
//                              FUNCTIONS
// ======================================================================

/**
 * @brief Create a message object to be sent over the internal network.
 *
 * @param[in] dest              The destination of the message. Can be a mask over multiple targets.
 * @param[in] src               The source (itself) of the message.
 * @param[in] type              The type of the message.
 * @param[in] info              Free 16 bits field of the message.
 * @param[in] payload           Pointer to a payload.
 * @param[in] payload_size      Number of bytes of the payload.
 *
 * @warning     This function allocate a buffer and copy the payload into it. That mean :
 *                  - the payload can be locally defined
 *                  - the payload can be changed after the function call, this won't affect the data.
 *
 * @return muxerInput_t *       A pointer to a structure, filled as requested.
 */
struct muxerInput_t *create_message(muxerDestination_t dest,
                                    muxerSource_t src,
                                    muxerMessageType_t type,
                                    uint16_t info,
                                    void *payload,
                                    uint32_t payload_size);

/**
 * @brief Consume a message that we received.
 *
 * @warning     This function will FREE the payload AND the message if we're the last client.
 *              Therefore, this function must be called once all the processing has been done !
 *
 * @param[in] msg   A pointer to the received message.
 *
 * @return ULONG
 * @retval TX_SUCESS    Everything when fine.
 * @retval TX_ERROR     Something failed.
 */
ULONG consume_message(struct muxerInput_t *msg);

/**
 * @brief Create a response message object to be sent over the internal network, as a response of a previous message.
 *        Some parameter can be changed, as part of the dedicated communication protocol between the two threads.
 *
 * @param[in] msg               A pointer to the received message.
 * @param[in] info              An info field to be updated in the output struct.
 * @param[in] payload           An optionnal new payload to be sent.
 * @param[in] payload_size      The payload size.
 *
 * @warning     Even if it may be tempting, the function allocate behind the scene a new structure and merge the data
 *              from the input message and provided by the user. At the end, the input message is freed to empty the
 *              memory pool.
 *
 * @return muxerInput_t* A pointer to a message, ready to be sent.
 */
struct muxerInput_t *
create_response_message(struct muxerInput_t *msg, uint16_t info, void *payload, uint32_t payload_size);

/**
 * @brief This function is the thread entry task. Do not call by yourself.
 *
 * @param arg Unused
 */
void muxer_task(ULONG arg);

#ifdef __cplusplus
}
#endif
