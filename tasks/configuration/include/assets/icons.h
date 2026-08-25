/**
 * @file    icons.h
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Define the icons to be used for the I2C screen.
 * @version 0.1
 * @date    2026-08-25
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

// ======================================================================
//                              INCLUDES
// ======================================================================
#include <stdint.h>

// ======================================================================
//                              Fonts
// ======================================================================
typedef struct {
    const uint8_t *data;
    uint16_t width;
    uint16_t height;
    uint8_t dataSize;
} tImage;

typedef struct {
    long int code;
    const tImage *image;
} tChar;

typedef struct {
    int length;
    const tChar *chars;
} tFont;