/**
 * @file    app_usbx_tasks.h
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Define the tasks that runs the USBX jobs.
 * @version 0.1
 * @date 2026-03-04
 *
 * @copyright Copyright (c) 2026
 *
 */

// ======================================================================
//                              INCLUDES
// ======================================================================
// USBX Core Headers
#include "tx_api.h"
#include "ux_api.h"

// ======================================================================
//                              FUNCTIONS
// ======================================================================

/**
 * @brief Provide the loop that runs for the terminal reception task.
 *
 * @param arg Unused.
 */
void usbx_terminal_recv_task(ULONG arg);

/**
 * @brief Provide the loop that runs for the bridge reception task.
 *
 * @param arg Unused.
 */
void usbx_bridge_recv_task(ULONG arg);

/**
 * @brief Provide the loop that runs for the programmer reception task.
 *
 * @param arg Unused.
 */
void usbx_programmer_recv_task(ULONG arg);
