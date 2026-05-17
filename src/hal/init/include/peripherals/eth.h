/**
 * @file    eth.h
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Ethernet peripheral init code
 * @version 0.1
 * @date    2026-03-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ======================================================================
//                              INCLUDE
// ======================================================================
#include "main.h"

extern ETH_HandleTypeDef heth;

// ======================================================================
//                              FUNCTIONS
// ======================================================================

void MX_ETH_Init(void);

#ifdef __cplusplus
}
#endif
