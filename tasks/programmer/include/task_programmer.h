/**
 * @file    task_programmer.h
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Define the programmer main task.
 * @version 0.1
 * @date    2026-08-31
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ======================================================================
//                               DEFINES
// ======================================================================
#define NET_PORT_GDB 2159
#define NET_PORT_XVC 2542
#define NET_PORT_SVF 4000

// ======================================================================
//                              INCLUDES
// ======================================================================
// Local library
#include "app_threadx.h"

// ======================================================================
//                              FUNCTIONS
// ======================================================================

void programmer_task(ULONG arg);

#ifdef __cplusplus
}
#endif
