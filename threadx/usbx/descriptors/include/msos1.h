/**
 * @file msos1.h
 * @author lheywang (leonard.heywang@proton.me)
 * @brief
 * @version 0.1
 * @date 2026-03-22
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once
#include <stdint.h>

/*
 * ------------------------------------
 * BASE DESCRIPTOR
 * ------------------------------------
 */
/*
 * The MS-OS 1.0 function specification.
 */
typedef struct __attribute__((packed)) {
    uint8_t bFirstInterfaceNumber;
    uint8_t reserved1;
    uint8_t compatibleID[8];
    uint8_t subCompatibleID[8];
    uint8_t reserved2[6];
} ms_os_10_function_t;

/*
 * The MS-OS 1.0 base descriptor.
 */
typedef struct __attribute__((packed)) {
    uint32_t dwLength;
    uint16_t bcdVersion;
    uint16_t wIndex;
    uint8_t bCount;
    uint8_t Reserved[7];
    ms_os_10_function_t function0;
} ms_os_10_compat_id_t;

/*
 * ------------------------------------
 * NAME DESCRIPTOR
 * ------------------------------------
 */
typedef struct __attribute__((packed)) {
    uint32_t dwSize;
    uint32_t dwPropertyDataType;
    uint16_t wPropertyNameLength;
    uint16_t PropertyName[13];
    uint32_t dwPropertyDataLength;
    uint16_t PropertyData[20];
} ms_os_10_property_terminal_t;

/*
 * Building the standard buffer. The remaining will be done dynamically.
 */
typedef struct __attribute__((packed)) {
    uint32_t dwLength;
    uint16_t bcdVersion;
    uint16_t wIndex;
    uint16_t wCount;
    uint32_t dwSize;
    uint32_t dwPropertyDataType;
    uint16_t wPropertyNameLength;
    uint16_t PropertyName[13];
    uint32_t dwPropertyDataLength;
    // The remaining data will be added by the CPU at the end of it.
} ms_os_10_header_t;

/*
 * ------------------------------------
 * EXTERN VALUES
 * ------------------------------------
 */
extern const ms_os_10_compat_id_t ms_os_10_descriptor;