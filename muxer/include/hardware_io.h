/**
 * @file    hardware_io.h
 * @author  l.heywang <leonard.heywang@proton.me>
 * @brief   Define the structures to sent commands to the hardware process.
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
//                              HARDWARE
// ======================================================================
// Enums
enum CMD_HARDWARE_TYPE {
    CMD_HARDWARE_TYPE_CONFIG,
    CMD_HARDWARE_TYPE_INPUT,
    CMD_HARDWARE_TYPE_INPUT_BLOCKING,
    CMD_HARDWARE_TYPE_OUTPUT,
    CMD_HARDWARE_TYPE_PREEMPT,
    CMD_HARDWARE_TYPE_RELEASE
};

enum CMD_HARDWARE_INFO {
    CMD_HARDWARE_INFO_USART,
    CMD_HARDWARE_INFO_SPI,
    CMD_HARDWARE_INFO_I2C,
    CMD_HARDWARE_INFO_I3C,
    CMD_HARDWARE_INFO_CAN,
    CMD_HARDWARE_INFO_ANALOG,
    CMD_HARDWARE_INFO_GPIO
};

/**
 * @brief   Define the order types for the Hardware command.
 *          Concatenate all types of workers.
 *
 */
struct HardwareCmd_t {
    uint32_t speed;
    uint8_t index;

    union {
        struct {
            uint8_t dataWidth;
            uint8_t stopBits;
            uint8_t parity;
        } usart;

        struct {
            uint8_t addr;
        } i2c;

        struct {
            uint8_t mode;
            uint8_t addr;
        } spi;

        struct {
            uint8_t pin;
            uint8_t state;
        } gpio;

        struct {
            uint32_t id;
            uint8_t type;
            uint8_t dlcSize;
        } can;

        struct {
            uint8_t pin;
            uint8_t speed;
        } analog;

    } params;

    uint8_t len;
    uint8_t payload[128];
};

#ifdef __cplusplus
}
#endif