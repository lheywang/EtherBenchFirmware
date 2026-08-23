/**
 * @file    ShellStreamCdc.cpp
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Implementation of the ShellStreamCdc class.
 * @version 0.1
 * @date    2026-03-06
 *
 * @copyright Copyright (c) 2026
 *
 */

#define LOG_MODULE "USB-BRIDGE"
// ======================================================================
//                              INCLUDE
// ======================================================================
// Header
#include "ShellStream/ShellStreamCdc.hpp"

// Local Libraries
#include "logger.h"
#include "task_shell.h"

// RTOS
#include "tx_api.h"
#include "ux_api.h"
#include "ux_device_class_cdc_acm.h"

// ======================================================================
//                            CLASS IMPL
// ======================================================================

void ShellStreamCdc::hw_transmit(const char *data, size_t length) {
    if (cdc_instance) {

        ULONG actual_length = 0;
        UINT status;

        const size_t MAX_PACKET_SIZE = 64;

        size_t bytes_remaining = length;
        uint8_t *current_ptr = (uint8_t *)data;

        while (bytes_remaining > 0) {

            // Get the remaining size to be sent
            size_t chunk_size = (bytes_remaining > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : bytes_remaining;

            status = ux_device_class_cdc_acm_write(this->cdc_instance, current_ptr, chunk_size, &actual_length);

            if (status != UX_SUCCESS) {
                LOG("Failed to send data. Error code is %08x", status);

                // Leave to never block anything.
                break;
            }

            // Update the pointer
            current_ptr += actual_length;
            bytes_remaining -= actual_length;
        }
    }
    return;
}

void ShellStreamCdc::connect(UX_SLAVE_CLASS_CDC_ACM *cdc) {
    // Update class settings
    this->cdc_instance = cdc;
    this->welcome_pending = true;

    // Add the welcome message
    // this->transmit(ShellWelcome);
    // this->transmit(ShellNewLine);
    return;
}

void ShellStreamCdc::disconnect() {
    this->cdc_instance = nullptr;
    return;
}

void ShellStreamCdc::process(const char *c, const size_t len) {

    // if required, already send the buffer.
    if (this->welcome_pending) {
        this->welcome_pending = false;
        this->transmit(ShellWelcome);
        this->transmit(ShellNewLine);
    }

    ShellStreamBase::process(c, len);
}

void ShellStreamCdc::process(const char *str) {

    // if required, already send the buffer.
    if (this->welcome_pending) {
        this->welcome_pending = false;
        this->transmit(ShellWelcome);
        this->transmit(ShellNewLine);
    }

    ShellStreamBase::process(str);
}