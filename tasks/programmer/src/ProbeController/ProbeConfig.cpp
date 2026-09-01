/**
 * @file    ProbeConfig.cpp
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Configure the ProbeController classe on the hardware side.
 *
 * @version 0.1
 * @date    2026-09-01
 *
 * @copyright Copyright (c) 2026
 *
 */

// ======================================================================
//                               INCLUDES
// ======================================================================
// Header
#include "ProbeController/ProbeConfig.hpp"

// HAL
#include "stm32h5xx_hal.h"
#include "stm32h5xx_ll_dma.h"
#include "stm32h5xx_ll_gpio.h"
#include "stm32h5xx_ll_rcc.h"
#include "stm32h5xx_ll_spi.h"

// STD
#include <cstdint>

// ======================================================================
//                               VARIABLES
// ======================================================================
ProbeBoardConfig configV1 = {

    .spim = {
        .instance = SPI1,
        .sclk = {GPIOG, LL_GPIO_PIN_9, LL_GPIO_AF_5},
        .mosi = {GPIOA, LL_GPIO_PIN_5, LL_GPIO_AF_5},
        .miso = {nullptr, 0, 0},
        .tx_dma = {GPDMA1, LL_DMA_CHANNEL_1, LL_GPDMA1_REQUEST_SPI1_TX},
        .rx_dma = {GPDMA1, LL_DMA_CHANNEL_1, LL_GPDMA1_REQUEST_SPI1_RX},
        .enableClock = []() { __HAL_RCC_SPI1_CLK_ENABLE(); },
        .disableClock = []() { __HAL_RCC_SPI1_CLK_DISABLE(); },
    },

    .spis = {
        .instance = SPI3,
        .sclk = {GPIOB, LL_GPIO_PIN_3, LL_GPIO_AF_6},
        .mosi = {GPIOB, LL_GPIO_PIN_5, LL_GPIO_AF_7},
        .miso = {GPIOB, LL_GPIO_PIN_4, LL_GPIO_AF_6},
        .tx_dma = {GPDMA1, LL_DMA_CHANNEL_2, LL_GPDMA1_REQUEST_SPI3_TX},
        .rx_dma = {GPDMA1, LL_DMA_CHANNEL_3, LL_GPDMA1_REQUEST_SPI3_TX},
        .enableClock = []() { __HAL_RCC_SPI3_CLK_ENABLE(); },
        .disableClock = []() { __HAL_RCC_SPI3_CLK_DISABLE(); },
    },

    .reset_pin = {GPIOC, LL_GPIO_PIN_6},
    .key_pin = {GPIOB, LL_GPIO_PIN_13},
    .detect_pin = {GPIOC, LL_GPIO_PIN_7},
};