/**
 * @file    task_programmer.cpp
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Implement the programmer main task.
 * @version 0.1
 * @date    2026-08-31
 *
 * @copyright Copyright (c) 2026
 *
 */
#define LOG_MODULE "Programmer"

// ======================================================================
//                              INCLUDES
// ======================================================================
// Header
#include "task_programmer.h"

// Other headers
#include "ProbeController/ProbeController.hpp"
#include "logger.h"

// STD
#include <cstdint>

// ======================================================================
//                            FUNCTIONS
// ======================================================================

void programmer_task(ULONG arg) {
    TX_PARAMETER_NOT_USED(arg);

    /*
     * First, init the hardware before passing it to the different classes :
     */
    ProbeController Probe = ProbeController();
    Probe.configureMode(TARGET_PROTOCOL_SWD);

    uint32_t current_freq = Probe.setBusFrequency(28000000);
    LOG("Configured Probe frequency to %d kHz", current_freq / 1000);

    /*
     * That's just an infinite loop.
     */
    while (1) {
        tx_thread_sleep(50);
    }
}