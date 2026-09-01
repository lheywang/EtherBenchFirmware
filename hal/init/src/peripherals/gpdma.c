/**
 * @file    gpdma.c
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   GPDMA init
 * @version 0.1
 * @date    2026-03-11
 *
 * @copyright Copyright (c) 2026
 *
 */

// ======================================================================
//                              INCLUDE
// ======================================================================
#include "gpdma.h"
#include "stm32h563xx.h"
#include "stm32h5xx_hal_dma.h"

#include <string.h>

DMA_HandleTypeDef hdma_usart3_tx;

// ======================================================================
//                              FUNCTIONS
// ======================================================================

void MX_GPDMA1_Init(void) {

    /* Peripheral clock enable */
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    /* GPDMA1 interrupt Init */
    // USART3 TX --> Logger
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 6, 3);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

    // SPI1 & 3 Init --> Programmer feature
    HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 6, 1);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);
    HAL_NVIC_SetPriority(GPDMA1_Channel2_IRQn, 6, 1);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel2_IRQn);
    HAL_NVIC_SetPriority(GPDMA1_Channel3_IRQn, 6, 1);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel3_IRQn);

    //  OCTOSPI --> NAND Flash
    HAL_NVIC_SetPriority(GPDMA1_Channel4_IRQn, 6, 2);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel4_IRQn);
    HAL_NVIC_SetPriority(GPDMA1_Channel5_IRQn, 6, 2);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel5_IRQn);

    // Still 2 channel free

    return;
}

void MX_GPDMA2_Init(void) {

    /* Peripheral clock enable */
    __HAL_RCC_GPDMA2_CLK_ENABLE();

    /* GPDMA2 interrupt Init */
    // WS2812 leds
    HAL_NVIC_SetPriority(GPDMA2_Channel0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(GPDMA2_Channel0_IRQn);

    // Hardware IOs
    // 3 USARTs
    // 1 SPI
    // 1 I2C/I3C
    // 1 CAN

    // Analog

    return;
}
