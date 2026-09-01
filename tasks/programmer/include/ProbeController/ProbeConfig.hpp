/**
 * @file    ProbeConfig.hpp
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Configure the ProbeController classe on the hardware side.
 *
 * @version 0.1
 * @date    2026-09-01
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

// ======================================================================
//                               INCLUDES
// ======================================================================
// HAL
#include "stm32h5xx_hal.h"
#include "stm32h5xx_ll_dma.h"
#include "stm32h5xx_ll_gpio.h"
#include "stm32h5xx_ll_spi.h"

// STD
#include <cstdint>

// ======================================================================
//                               STRUCTS
// ======================================================================
struct GpioPinDef {
    GPIO_TypeDef *port;
    uint32_t pin;
    uint32_t af;
};

struct DmaChannelDef {
    DMA_TypeDef *dma;
    uint32_t channel;
    uint32_t request;
};

struct SpiInterfaceDef {
    SPI_TypeDef *instance;
    GpioPinDef sclk;
    GpioPinDef mosi;
    GpioPinDef miso;
    DmaChannelDef tx_dma;
    DmaChannelDef rx_dma;

    void (*enableClock)();
    void (*disableClock)();
};

struct ProbeBoardConfig {
    SpiInterfaceDef spim;
    SpiInterfaceDef spis;
    GpioPinDef reset_pin;
    GpioPinDef key_pin;
    GpioPinDef detect_pin;
};

// ======================================================================
//                               VARIABLES
// ======================================================================
extern ProbeBoardConfig configV1;