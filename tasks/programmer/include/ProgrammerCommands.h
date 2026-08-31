/**
 * @file    ProgrammerCommands.hpp
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Define the commands the programmer does understand. This act as an interface for the high level servers that
 *          runs as separated threads.
 * @version 0.1
 * @date    2026-08-31
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

// ======================================================================
//                               INCLUDES
// ======================================================================
// RTOS
#include "tx_api.h"

// STD
#include <stdint.h>

// ======================================================================
//                               ENUMS
// ======================================================================
/**
 * @brief Commands to be sent by the different servers to the TargetController.
 *
 */
typedef enum {
    /*
     * Basic commands to handle the life cycle.
     */
    TARGET_CMD_NONE,
    TARGET_CMD_ATTACH,
    TARGET_CMD_DETACH,
    TARGET_CMD_RESET,
    TARGET_CMD_READ_COREID,

    /*
     * Commands to the handle the target CPU
     */
    TARGET_CMD_HALT,
    TARGET_CMD_RESUME,
    TARGET_CMD_STEP,
    TARGET_CMD_GET_STATE,

    /*
     * Commands to access the CPU Registers
     */
    TARGET_CMD_READ_REG,
    TARGET_CMD_WRITE_REG,
    TARGET_CMD_READ_ALL_REGS,
    TARGET_CMD_WRITE_ALL_REGS,

    /*
     * Commands to access the Target memory
     */
    TARGET_CMD_READ_MEM,
    TARGET_CMD_WRIE_MEM,

    /*
     * Commands to use breakpoints and / or watchpoints.
     */
    TARGET_CMD_SET_BKPT,
    TARGET_CMD_CLEAR_BKPT,
    TARGET_CMD_SET_WTCHPT,
    TARGET_CMD_CLEAR_WTCHPT,

    /*
     * Commands to handle the flash of the device
     */
    TARGET_CMD_MASS_ERASE,
    TARGET_CMD_SECTOR_ERASE,
    TARGET_CMD_PROGRAM_BLOCK
} TargetCommandType;

/**
 * @brief Target status. Returned at any point by the controller classes.
 *
 */
typedef enum {
    TARGET_STATE_UNKNOWN,
    TARGET_STATE_RESET,
    TARGET_STATE_RUNNING,
    TARGET_STATE_HALTED,
    TARGET_STATE_SLEEPING,
    TARGET_STATE_ERROR
} TargetState;

/**
 * @brief Target results. Returned by the protocol Target to handle the different elements.
 *
 */
typedef enum {
    TARGET_RES_OK,
    TARGET_RES_ERROR,
    TARGET_RES_TIMEOUT,
    TARGET_RES_NOT_CONNECTED,
    TARGET_RES_UNSUPPORTED,
    TARGET_RES_MEM_FAULT,
    TARGET_RES_BUSY,
} TargetResult;

/**
 * @brief Target protocol to be used. This indicate which protocol handler will be called.
 *        Please note : Not all of them may be present, therefore check the logs !
 */
typedef enum {
    TARGET_PROTOCOL_NONE,
    TARGET_PROTOCOL_SWD,
    TARGET_PROTOCOL_SWDO,
    TARGET_PROTOCOL_JTAG,
    TARGET_PROTOCOL_cJAG,
    TARGET_PROTOCOL_ICSP,
    TARGET_PROTOCOL_SPI
} TargetProtocol;

/**
 * @brief Target command source. Used by te TargetController classe to handle the mutexes.
 *        Please note : Only this field is used as source, therefore it's easy to bypass the mutex.
 *        As this struct shall not be accessible to the user under any circumstances, this is acceptable.
 */
typedef enum { TARGET_SRC_NONE, TARGET_SRC_GDB, TARGET_SRC_CMSIS } TargetCommandSource;

// ======================================================================
//                               STRUCTS
// ======================================================================
typedef struct {

    /*
     * Command Infos
     */
    TargetCommandSource source;

    /*
     * Basic IO
     */
    TargetCommandType command;
    TargetResult result;
    TargetState state;
    TargetProtocol protocol;

    /*
     * Payload
     */
    uint32_t address;
    void *payload;
    size_t size;

    /*
     * Return channel
     */
    TX_QUEUE *response;

} ProgrammerRequest;