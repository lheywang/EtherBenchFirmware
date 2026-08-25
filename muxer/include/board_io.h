/**
 * @file    board_io.h
 * @author  l.heywang <leonard.heywang@proton.me>
 * @brief   Define the structures used by the different process to
 *          communicate. This ensure a consistency between the transfers,
 *          preventing from magic numbers to be sent.
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
// STD
#include "stdint.h"
#include "stdio.h"

// ======================================================================
//                              BOARD
// ======================================================================
// Enums
enum CMD_BOARD_TYPE {
    CMD_BOARD_TYPE_CONFIG,
    CMD_BOARD_TYPE_INPUT,
    CMD_BOARD_TYPE_INPUT_BLOCKING,
    CMD_BOARD_TYPE_OUTPUT
};

enum CMD_BOARD_INFO {
    CMD_BOARD_INFO_POWER,
    CMD_BOARD_INFO_SCREEN,
    CMD_BOARD_INFO_TEMPERATURE,
};

/**
 * @brief   Define the order types for the Board command.
 *          Concatenate all types of workers.
 *
 */
struct BoardCmd_t {

    uint8_t len;
    uint8_t payload[128];
};

#ifdef __cplusplus
}
#endif