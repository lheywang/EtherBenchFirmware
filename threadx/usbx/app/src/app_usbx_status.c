/**
 * @file    app_usbx_status.c
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Implement changement handlers for USBX statuses.
 * @version 0.1
 * @date    2026-03-04
 *
 * @copyright Copyright (c) 2026
 *
 */

// ======================================================================
//                              INCLUDES
// ======================================================================
// Header
#include "app_usbx_status.h"

// Local libraries
#define LOG_MODULE "USBX_STATUS"
#include "descriptor_config.h"
#include "logger.h"
#include "msos1.h"
#include "task_muxer.h"

// USBX Core Headers
#include "tx_api.h"
#include "ux_api.h"
#include "ux_dcd_stm32.h"
#include "ux_device_class_cdc_acm.h"
#include "ux_device_class_dpump.h"
#include "ux_device_class_storage.h"
#include "ux_device_stack.h"

// ThreadX
#include "tx_handler.h"

// Callbacks from the shell interface to the parser.
extern void usb_push_data(const uint8_t *data, const uint32_t len);
extern void usb_connect(UX_SLAVE_CLASS_CDC_ACM *cdc_instance);
extern void usb_disconnect();

// Private functions
static UINT build_microsoft_friendly_name(const uint16_t *friendly_name,
                                          size_t name_size,
                                          ULONG length,
                                          UCHAR *data_buffer,
                                          ULONG *data_length);

// ======================================================================
//                              VARIABLES
// ======================================================================
// Flags
TX_EVENT_FLAGS_GROUP usbx_app_flags;

// Handles
UX_SLAVE_CLASS_CDC_ACM *usbx_terminal = UX_NULL;
UX_SLAVE_CLASS_CDC_ACM *usbx_bridge = UX_NULL;
UX_SLAVE_CLASS_DPUMP *usbx_cmsis = UX_NULL;

// Extern
extern TX_QUEUE router_input; // from launcher.c

// ======================================================================
//                              FUNCTIONS
// ======================================================================

UINT USBD_ChangeFunction(ULONG Device_State) {
    UINT status = UX_SUCCESS;

    switch (Device_State) {
    case UX_DEVICE_ATTACHED:
        tx_event_flags_set(&usbx_app_flags, USBX_STATUS_ATTACHED, TX_OR);
        LOG("USBX Device attached");
        break;

    case UX_DEVICE_REMOVED:
        tx_event_flags_set(&usbx_app_flags, ~USBX_STATUS_ATTACHED, TX_AND);
        LOG("USBX Device removed");
        break;

    case UX_DCD_STM32_DEVICE_CONNECTED:
        tx_event_flags_set(&usbx_app_flags, USBX_STATUS_CONNECTED, TX_OR);
        LOG("USBX Device connected");
        break;

    case UX_DCD_STM32_DEVICE_DISCONNECTED:
        tx_event_flags_set(&usbx_app_flags, ~USBX_STATUS_CONNECTED, TX_AND);
        LOG("USBX Device disconnected");
        break;

    case UX_DCD_STM32_DEVICE_SUSPENDED:
        tx_event_flags_set(&usbx_app_flags, USBX_STATUS_SUSPENDED, TX_OR);
        tx_event_flags_set(&usbx_app_flags, ~USBX_STATUS_RESUMED, TX_AND);
        LOG("USBX Device suspended");
        break;

    case UX_DCD_STM32_DEVICE_RESUMED:
        tx_event_flags_set(&usbx_app_flags, USBX_STATUS_RESUMED, TX_OR);
        tx_event_flags_set(&usbx_app_flags, ~USBX_STATUS_SUSPENDED, TX_AND);
        LOG("USBX Device resumed");
        break;

    case UX_DCD_STM32_SOF_RECEIVED:
        break;

    default:

        break;
    }

    return status;
}

/*
 * USB TERMINAL
 */
VOID USBX_TerminalDisable(VOID *cdc_instance) {

    // Set some flags
    usbx_terminal = (UX_SLAVE_CLASS_CDC_ACM *)cdc_instance;
    tx_event_flags_set(&usbx_app_flags, ~USBX_STATUS_CDC_TERMINAL_CONNECTED, TX_AND);
    usb_disconnect();
    return;
}

VOID USBX_TerminalEnable(VOID *cdc_instance) {

    // Set some flags
    usbx_terminal = (UX_SLAVE_CLASS_CDC_ACM *)cdc_instance;
    tx_event_flags_set(&usbx_app_flags, USBX_STATUS_CDC_TERMINAL_CONNECTED, TX_OR);
    return;
}

VOID USBX_TerminalChange(VOID *cdc_instance) {

    // Nothing to be done here !
    tx_event_flags_set(&usbx_app_flags, USBX_STATUS_CDC_TERMINAL_CHANGE_REQUIRED, TX_OR);

    usbx_terminal = (UX_SLAVE_CLASS_CDC_ACM *)cdc_instance;

    // Ensure our pointers are valids.
    if (cdc_instance == NULL)
        return;

    static UINT last_dtr_state = UX_FALSE;

    // Detect the change of the DTR state
    if (usbx_terminal->ux_slave_class_cdc_acm_data_dtr_state != last_dtr_state) {

        // Update the last known state.
        last_dtr_state = usbx_terminal->ux_slave_class_cdc_acm_data_dtr_state;

        if (last_dtr_state == UX_TRUE) {
            usb_connect(usbx_terminal);
            LOG("Terminal was openned.");
        }
    }

    // As that's a pure software port, we don't care about the update of the terminal.
    // These updates will be passed over software requests.

    return;
}

/*
 * USB-USART BRIDGE
 */
VOID USBX_USARTBridgeDisable(VOID *cdc_instance) {

    usbx_bridge = (UX_SLAVE_CLASS_CDC_ACM *)cdc_instance;
    tx_event_flags_set(&usbx_app_flags, ~USBX_STATUS_CDC_USB_USART_CONNECTED, TX_AND);
    return;
}

VOID USBX_USARTBridgeEnable(VOID *cdc_instance) {

    usbx_bridge = (UX_SLAVE_CLASS_CDC_ACM *)cdc_instance;
    tx_event_flags_set(&usbx_app_flags, USBX_STATUS_CDC_USB_USART_CONNECTED, TX_OR);
    return;
}

VOID USBX_USARTBridgeChange(VOID *cdc_instance) {
    // Setting up the required flags
    usbx_bridge = (UX_SLAVE_CLASS_CDC_ACM *)cdc_instance;
    tx_event_flags_set(&usbx_app_flags, USBX_STATUS_CDC_USB_USART_CHANGE_REQUIRED, TX_OR);

    // Ensure our pointers are valids.
    if (cdc_instance == NULL)
        return;

    static UINT last_dtr_state = UX_FALSE;

    // Detect the change of the DTR state
    if (usbx_bridge->ux_slave_class_cdc_acm_data_dtr_state != last_dtr_state) {

        // Update the last known state.
        last_dtr_state = usbx_bridge->ux_slave_class_cdc_acm_data_dtr_state;

        if (last_dtr_state == UX_TRUE) {
            // usb_connect(usbx_bridge);
            LOG("Bridge was openned.");
        }
    }

    // As that's a pure software port, we don't care about the update of the terminal.
    // These updates will be passed over software requests.

    uint32_t payload[4] = {(uint32_t)usbx_terminal->ux_slave_class_cdc_acm_baudrate,
                           (uint32_t)usbx_terminal->ux_slave_class_cdc_acm_data_bit,
                           (uint32_t)usbx_terminal->ux_slave_class_cdc_acm_stop_bit,
                           (uint32_t)usbx_terminal->ux_slave_class_cdc_acm_parity};

    // Build the message payload.
    struct muxerInput_t *command =
        create_message(hardwareDestination, usbSource, 0x0001, 0x0000, &payload, sizeof(payload));
    if (command == NULL) {
        LOG("An error occured while assembling the payload for the router.");
        return;
    }

    // Send the messageto the Queue. If fail, consume it to free it's owned memory.
    if (tx_queue_send(&router_input, &command, TX_NO_WAIT) != TX_SUCCESS) {
        LOG("Could not send the config message. Dropping the packet.");
        consume_message(command);
    }

    LOG("USBX Changed bridge parameters");
    return;
}

/*
 * CMSIS-DEBUGGER
 */
VOID USBX_CMSISDisable(VOID *cmsis_instance) {

    usbx_cmsis = (UX_SLAVE_CLASS_DPUMP *)cmsis_instance;
    tx_event_flags_set(&usbx_app_flags, ~USBX_STATUS_CMSIS_CONNECTED, TX_AND);
    LOG("USBX Disabled CMSIS");
    return;
}

VOID USBX_CMSISEnable(VOID *cmsis_instance) {

    usbx_cmsis = (UX_SLAVE_CLASS_DPUMP *)cmsis_instance;
    tx_event_flags_set(&usbx_app_flags, USBX_STATUS_CMSIS_CONNECTED, TX_OR);
    LOG("USBX Enabled CMSIS");
    return;
}

/*
 * MS OS Descriptor handler.
 */
UINT etherbench_ms_vendor_request_handler(ULONG request,
                                          ULONG value,
                                          ULONG index,
                                          ULONG length,
                                          UCHAR *data_buffer,
                                          ULONG *data_length) {

    // Unused
    (void)request;
    (void)value;

    ULONG response_len = 0;
    ULONG interface_number = 0;

    /* Is windows requesting the Extended Compat ID ? */
    switch (index) {
    case 0x0004:

        /* Fetch the response size of our descriptor. */
        response_len = sizeof(ms_os_10_compat_id_t);

        if (response_len > length) {
            response_len = length;
        }

        /* Copy our buffer to the USBX internal buffer. */
        ux_utility_memory_copy(data_buffer, (UCHAR *)&ms_os_10_descriptor, response_len);

        /* Updating the size for USBX*/
        *data_length = response_len;

        /* Ensure some logging to be done !*/
        LOG("Windows asked 0x0004. Sent our descriptor.");

        return UX_SUCCESS;
        break;

    case 0x0005:

        /*
         * Calling the right function, depending on the index that was used.
         */
        interface_number = value & 0xFF;

        // Some Usart logs.

        switch (interface_number) {

        case 0x00: // Terminal CDC

            LOG("Windows asked 0x%04x for interface %d", index, interface_number);
            return build_microsoft_friendly_name(
                STR_COM1_MICROSOFT, sizeof(STR_COM1_MICROSOFT), length, data_buffer, data_length);
            break;

        case 0x02: // USB-USART Bridge

            LOG("Windows asked 0x%04x for interface %d", index, interface_number);
            return build_microsoft_friendly_name(
                STR_COM2_MICROSOFT, sizeof(STR_COM2_MICROSOFT), length, data_buffer, data_length);
            break;

        case 0x04: // CMSIS-DAP

            LOG("Windows asked 0x%04x for interface %d", index, interface_number);
            return build_microsoft_friendly_name(
                STR_DBG1_MICROSOFT, sizeof(STR_DBG1_MICROSOFT), length, data_buffer, data_length);
            break;

        default:
            LOG("Windows asked 0x%04x for interface %d which is unknown to us.", index, interface_number);
            return UX_ERROR;
            break;
        }

        break;

    default:

        /* Ensure some logging to be done !*/
        LOG("Windows asked 0x%04x. Unknown request.", index);

        /* Unsupported request. */
        return UX_ERROR;
        break;
    }
}

static UINT build_microsoft_friendly_name(const uint16_t *friendly_name,
                                          size_t name_size,
                                          ULONG length,
                                          UCHAR *data_buffer,
                                          ULONG *data_length) {

    /*
     * First, fetch the standard buffer :
     */
    ms_os_10_header_t header = {
        .dwLength = sizeof(ms_os_10_header_t) + name_size,
        .bcdVersion = 0x0100,
        .wIndex = 0x0005,
        .wCount = 0x0001,
        .dwSize = 40 + name_size,
        .dwPropertyDataType = 0x00000001,
        .wPropertyNameLength = sizeof(u"FriendlyName"),
        .PropertyName = u"FriendlyName",
        .dwPropertyDataLength = name_size,
    };

    ULONG total_size = header.dwLength;

    /*
     * Making sure we have enough memory to work
     */
    if (total_size > length) {
        total_size = length;
    }

    /*
     * First, copy the header into the memory
     */
    ux_utility_memory_copy(data_buffer, (UCHAR *)&header, sizeof(ms_os_10_header_t));

    /*
     * Copying the payload right at the end of the buffer, where it shall be.
     */
    _ux_utility_memory_copy(data_buffer + sizeof(ms_os_10_header_t), (UCHAR *)friendly_name, name_size);

    /*
     * Updating the pointer
     */
    *data_length = total_size;

    return UX_SUCCESS;
}