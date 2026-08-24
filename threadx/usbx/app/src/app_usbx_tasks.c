/**
 * @file    app_usbx_tasks.c
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Define the tasks that handle the USB communications.
 * @version 0.1
 * @date    2026-08-23
 *
 * @copyright Copyright (c) 2026
 *
 */

// ======================================================================
//                              INCLUDES
// ======================================================================
// Header
#include "app_usbx_tasks.h"

// Logger
#define LOG_MODULE "USBX-TASKS"
#include "logger.h"

// Local libraries
#include "app_usbx_status.h"
#include "bridge/usb_bridge.h"
#include "task_muxer.h"

// USBX Core Headers
#include "tx_api.h"
#include "ux_api.h"

// ======================================================================
//                              VARIABLES
// ======================================================================

// Reception buffers
static __attribute__((aligned(32))) UCHAR usbx_terminal_buffer[64];
static __attribute__((aligned(32))) UCHAR usbx_bridge_buffer[64];
static __attribute__((aligned(32))) UCHAR usbx_programmer_buffer[64];

// Extern
extern TX_QUEUE router_input; // from launcher.c

// ======================================================================
//                              FUNCTIONS
// ======================================================================

void usbx_terminal_recv_task(ULONG arg) {

    (void)arg;

    ULONG current_length;
    UINT status;

    while (1) {

        // First, check that we CAN read.
        if (usbx_terminal == NULL) {
            tx_thread_sleep(10);
            continue;
        }

        // Wait until we're enabled ...
        ULONG flags = 0;
        tx_event_flags_get(&usbx_app_flags, USBX_STATUS_CDC_TERMINAL_CONNECTED, TX_AND, &flags, TX_WAIT_FOREVER);

        // Then, we can read
        status = ux_device_class_cdc_acm_read(
            usbx_terminal, usbx_terminal_buffer, sizeof(usbx_terminal_buffer), &current_length);

        // If we read something, send it to the parser.
        if ((status == UX_SUCCESS) && (current_length > 0)) {
            usb_push_data(usbx_terminal_buffer, current_length);
        }
    }
}

void usbx_bridge_recv_task(ULONG arg) {

    (void)arg;

    ULONG current_length;
    UINT status;

    while (1) {

        // First, check that we CAN read.
        if (usbx_bridge == NULL) {
            tx_thread_sleep(10);
            continue;
        }

        // Wait until we're enabled ...
        ULONG flags = 0;
        tx_event_flags_get(&usbx_app_flags, USBX_STATUS_CDC_USB_USART_CONNECTED, TX_AND, &flags, TX_WAIT_FOREVER);

        // Then, we can read
        status =
            ux_device_class_cdc_acm_read(usbx_bridge, usbx_bridge_buffer, sizeof(usbx_bridge_buffer), &current_length);

        // If we read something, send it to the parser.
        if ((status == UX_SUCCESS) && (current_length > 0)) {

            // We received some data, therefore we need to push it to the main router.
            struct muxerInput_t *cmd =
                create_message(hardwareDestination, usbSource, 0x0000, 0x0000, &usbx_bridge_buffer, current_length);

            if (cmd != NULL) {
                if (tx_queue_send(&router_input, cmd, TX_NO_WAIT) != TX_SUCCESS) {
                    consume_message(cmd);
                }
            }
        }
    }
}

void usbx_programmer_recv_task(ULONG arg) {}
