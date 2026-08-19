/**
 * @file    launcher.c
 * @author  l.heywang <leonard.heywang@proton.me>
 * @brief   Header file for the global launcher. Create the tasks and
 * 			memory regions.
 * @version 0.1
 * @date    2026-02-21
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

// ======================================================================
//                              INCLUDES
// ======================================================================
// STD
#include <stdint.h>

// ======================================================================
//                              DEFINES
// ======================================================================
// STACK SIZES
#define IDLE_STACK_SIZE       1024
#define LOGGER_STACK_SIZE     1536
#define PARSER_STACK_SIZE     2024
#define ROUTER_STACK_SIZE     1024
#define BOARDCFG_STACK_SIZE   1024
#define HARDWARE_STACK_SIZE   2048
#define PROGRAMMER_STACK_SIZE 2048
#define SEQUENCER_STACK_SIZE  1536
#define ARGB_STACK_SIZE       1024

// CLIENT FIFO DEPTHS
#define CLIENT_FIFO_DEPTH 16

// ======================================================================
//                              FUNCTIONS
// ======================================================================

/**
 * @brief 	This function is charged to initialize the memory areas and
 * 			launch the different tasks needed.
 *
 * 			This enable to perform a clear separation of the kernel
 * space, and the user space.
 *
 * 			Automatically called from tx_application_define from the
 * 			threadX init procedure.
 *
 * @return	uint32_t status.
 */
uint32_t launcher(void);
