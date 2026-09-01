/**
 * @file    ProbeController.cpp
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Define the probe low level actions, used to by any probe classes.
 * @version 0.1
 * @date    2026-08-31
 *
 * @copyright Copyright (c) 2026
 *
 */
#define LOG_MODULE "ProbeController"

// ======================================================================
//                               INCLUDES
// ======================================================================
// Header
#include "ProbeController/ProbeController.hpp"

// Other headers
#include "ProgrammerCommands.h"
#include "logger.h"

// HAL
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_rcc.h"
#include "stm32h5xx_ll_dma.h"
#include "stm32h5xx_ll_gpio.h"
#include "stm32h5xx_ll_spi.h"

// RTOS
#include "tx_api.h"

// STD
#include <cstdint>
#include <span>

// ======================================================================
//                               VARIABLES
// ======================================================================
TX_SEMAPHORE programmer_dma;

// ======================================================================
//                            PUBLIC CLASS
// ======================================================================
ProbeController::ProbeController() {
    // Reset the mode
    this->mode = TargetProtocol::TARGET_PROTOCOL_NONE;

    // Init the basic hardware
    this->initGPIO();

    // Init the semaphore
    if (tx_semaphore_create(&programmer_dma, (CHAR *)"DMA", 0) != TX_SUCCESS) {
        LOG("Failed to initialize the DMA semaphore");
    }

    // Init the DMA
    this->initGPDMA();

    return;
}

ProbeController::~ProbeController() {
    // Reset the IO pins
    this->configureMode(TargetProtocol::TARGET_PROTOCOL_NONE);

    // Reset the GPIOs
    this->deinitGPIO();

    // Reset the GPDMA
    this->deinitGPDMA();

    // Delete the semaphore
    if (tx_semaphore_delete(&programmer_dma) != TX_SUCCESS) {
        LOG("Failed to delete the DMA semaphore");
    }

    return;
}

/*
 * Probe API
 */
bool ProbeController::setTargetPower(const bool enable) {
    (void)enable;
    LOG("Feature TargetPower is not currently available.");
    return false;
}

float ProbeController::getTargetVoltage() {
    LOG("Feature TargetVoltage is not currently available.");
    return 3.3;
}

bool ProbeController::setResetPin(const bool status) {
    if (status) {
        LL_GPIO_SetOutputPin(RESET_PORT, RESET_PIN);
        return true;
    }

    LL_GPIO_ResetOutputPin(RESET_PORT, RESET_PIN);
    return true;
}

bool ProbeController::configureMode(const TargetProtocol request) {

    // Check if the current mode is not already good ?
    if (this->mode == request) {
        return true;
    }

    // As required, deinit ourself.
    switch (this->mode) {
    case TargetProtocol::TARGET_PROTOCOL_JTAG:
        this->deinitAsJTAG();
        break;
    case TargetProtocol::TARGET_PROTOCOL_SWD:
        this->deinitAsSWD();
        break;
    case TargetProtocol::TARGET_PROTOCOL_SWDO:
        this->deinitAsSWDO();
        break;
    case TargetProtocol::TARGET_PROTOCOL_SPI:
        this->deinitAsSPI();
        break;
    default:
        break;
    }

    // Then, configure ourselves as the future config :
    switch (request) {
    case TargetProtocol::TARGET_PROTOCOL_JTAG:
        this->initAsJTAG();
        LOG("Changed the probe mode to JTAG");
        break;
    case TargetProtocol::TARGET_PROTOCOL_SWD:
        this->initAsSWD();
        LOG("Changed the probe mode to SWD");
        break;
    case TargetProtocol::TARGET_PROTOCOL_SWDO:
        this->initAsSWDO();
        LOG("Changed the probe mode to SWDO");
        break;
    case TargetProtocol::TARGET_PROTOCOL_SPI:
        this->initAsSPI();
        LOG("Changed the probe mode to SPI");
        break;
    default:
        break;
    }

    // Update the value
    this->mode = request;
    return true;
}

uint32_t ProbeController::setBusFrequency(const uint32_t frequency) {

    const uint32_t st_clock = 250000000;
    const uint32_t dividers[] = {4, 8, 16, 32, 64, 128, 256};

    /*
     * Fetch the two dividers that match the speed :
     */
    uint8_t current_div = 0;
    while ((st_clock / dividers[current_div]) > frequency) {
        current_div++;
        if (current_div == 6) {
            break;
        }
    }

    /*
     * Now, look for the best
     */
    const uint32_t hfreq = st_clock / dividers[current_div - 1];
    const uint32_t lfreq = st_clock / dividers[current_div];

    const uint32_t hdelta = hfreq - frequency;
    const uint32_t ldelta = frequency - lfreq;

    if (hdelta < ldelta) {
        current_div -= 1;
    }

    /*
     * Configure the peripheral
     */
    const uint32_t ll_dividers[] = {LL_SPI_BAUDRATEPRESCALER_DIV4,
                                    LL_SPI_BAUDRATEPRESCALER_DIV8,
                                    LL_SPI_BAUDRATEPRESCALER_DIV16,
                                    LL_SPI_BAUDRATEPRESCALER_DIV32,
                                    LL_SPI_BAUDRATEPRESCALER_DIV64,
                                    LL_SPI_BAUDRATEPRESCALER_DIV128,
                                    LL_SPI_BAUDRATEPRESCALER_DIV256};

    LL_SPI_Disable(SPIS_SPI_HANDLE);
    LL_SPI_Disable(SPIM_SPI_HANDLE);
    LL_SPI_SetBaudRatePrescaler(SPIS_SPI_HANDLE, ll_dividers[current_div]);
    LL_SPI_SetBaudRatePrescaler(SPIM_SPI_HANDLE, ll_dividers[current_div]);
    LL_SPI_Enable(SPIS_SPI_HANDLE);
    LL_SPI_Enable(SPIM_SPI_HANDLE);

    /*
     * Return the current direction
     */
    return st_clock / dividers[current_div];
}

bool ProbeController::toggleDirection() {

    switch (this->mode) {
    case TargetProtocol::TARGET_PROTOCOL_SWD:
    case TargetProtocol::TARGET_PROTOCOL_SWDO:
        // Just do a simple exange here. No more than 10 CPU cycles.
        LL_SPI_Disable(SPIM_SPI_HANDLE);
        if (LL_SPI_GetTransferDirection(SPIM_SPI_HANDLE) == LL_SPI_HALF_DUPLEX_TX) {
            LL_SPI_SetTransferDirection(SPIM_SPI_HANDLE, LL_SPI_HALF_DUPLEX_RX);
        } else {
            LL_SPI_SetTransferDirection(SPIM_SPI_HANDLE, LL_SPI_HALF_DUPLEX_TX);
        }
        LL_SPI_Enable(SPIM_SPI_HANDLE);
    default:
        // Nothing to do.
        break;
    }

    return true;
}

bool ProbeController::ioBits(std::span<const uint8_t> tms,
                             std::span<const uint8_t> tdi,
                             std::span<uint8_t> tdo,
                             size_t bitCount) {}

bool ProbeController::delayus(const uint32_t us) {

    /*
     * Compute the delay cycles needed
     */
    const uint32_t st_clock = 250000000;
    const uint32_t ticks = (1000 * us) / st_clock;

    /*
     * Wait. As the RTOS may preempt us here, the delay may, or may not be respected.
     */
    for (uint32_t tick = ticks; tick > 0; tick--) {
        __asm volatile("nop");
    }

    return true;
}

// ======================================================================
//                             PRIVATE CLASS
// ======================================================================
/* ----------------------------------------------------------------------
 * SWD
 * ----------------------------------------------------------------------
 */
void ProbeController::initAsSWD() {

    /*
     * Enable the clock
     */
    SPIM_CLOCK_ENABLE
    /*
     * First, configure the IO
     */
    LL_GPIO_InitTypeDef setup = {};
    setup.Mode = LL_GPIO_MODE_ALTERNATE;
    setup.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    setup.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    setup.Pull = LL_GPIO_PULL_NO;

    setup.Pin = SPIM_SCLK_PIN;
    setup.Alternate = SPIM_SCLK_AF;
    if (LL_GPIO_Init(SPIM_SCLK_PORT, &setup) != SUCCESS) {
        LOG("Failed to configure SPIM SCLK Pin.");
    }

    setup.Pin = SPIM_IO_PIN;
    setup.Alternate = SPIM_IO_AF;
    if (LL_GPIO_Init(SPIM_IO_PORT, &setup) != SUCCESS) {
        LOG("Failed to configure SPIM IO Pin.");
    }

    /*
     * Finally, init the peripherals :
     */
    LL_SPI_InitTypeDef spim_config = {};

    spim_config.TransferDirection = LL_SPI_HALF_DUPLEX_TX;
    spim_config.Mode = LL_SPI_MODE_MASTER;
    spim_config.DataWidth = LL_SPI_DATAWIDTH_8BIT; // Will be ajusted in any case !
    spim_config.ClockPolarity = LL_SPI_POLARITY_LOW;
    spim_config.ClockPhase = LL_SPI_PHASE_1EDGE;
    spim_config.NSS = LL_SPI_NSS_SOFT;
    spim_config.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV8; // 31.25 MHz
    spim_config.BitOrder = LL_SPI_MSB_FIRST;
    spim_config.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;

    if (LL_SPI_Init(SPIM_SPI_HANDLE, &spim_config) != SUCCESS) {
        LOG("Failed to init the SPIM peripheral");
    }

    LL_SPI_SetFIFOThreshold(SPIM_SPI_HANDLE, LL_SPI_FIFO_TH_01DATA);
    LL_SPI_Enable(SPIM_SPI_HANDLE);

    return;
}

void ProbeController::deinitAsSWD() {

    /*
     * Remove the pins from the peripheral
     */
    LL_GPIO_InitTypeDef setup = {};
    setup.Pull = LL_GPIO_PULL_NO;
    setup.Mode = LL_GPIO_MODE_ANALOG;

    setup.Pin = SPIM_SCLK_PIN;
    if (LL_GPIO_Init(SPIM_SCLK_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset SPIM SCLK Pin.");
    }

    setup.Pin = SPIM_IO_PIN;
    if (LL_GPIO_Init(SPIM_IO_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset SPIM IO Pin.");
    }

    /*
     * Disable the peripheral
     */
    LL_SPI_Disable(SPIM_SPI_HANDLE);

    /*
     * Disable the clocks
     */
    SPIM_CLOCK_DISABLE

    return;
}

/* ----------------------------------------------------------------------
 * SWD0
 * ----------------------------------------------------------------------
 */
void ProbeController::initAsSWDO() {

    // SWDO Mode share the same logic as SWD + the serial input.
    this->initAsSWD();
    SPIS_CLOCK_ENABLE

    /*
     * TODO : Config here on the final board, as these feature are only supported the BGA chip.
     */

    return;
}

void ProbeController::deinitAsSWDO() {

    // SWDO share the same logic as SWD + serial input.
    this->deinitAsSWD();

    // Then, reset the pin
    LL_GPIO_InitTypeDef setup = {};
    setup.Pull = LL_GPIO_PULL_NO;
    setup.Mode = LL_GPIO_MODE_ANALOG;

    setup.Pin = SPIS_MISO_PIN;
    if (LL_GPIO_Init(SPIS_MISO_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset SPIS MISO Pin.");
    }

    /*
     * Disable the peripheral
     */
    LL_SPI_Disable(SPIS_SPI_HANDLE);

    /*
     * Disable the clocks
     */
    SPIS_CLOCK_DISABLE

    return;
}

/* ----------------------------------------------------------------------
 * JTAG
 * ----------------------------------------------------------------------
 */
void ProbeController::initAsJTAG() {

    /*
     * Enable the clocks
     */
    SPIM_CLOCK_ENABLE
    SPIS_CLOCK_ENABLE

    /*
     * SPIM Pins config
     */
    LL_GPIO_InitTypeDef setup = {};
    setup.Mode = LL_GPIO_MODE_ALTERNATE;
    setup.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    setup.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    setup.Pull = LL_GPIO_PULL_NO;

    setup.Pin = SPIM_SCLK_PIN;
    setup.Alternate = SPIM_SCLK_AF;
    if (LL_GPIO_Init(SPIM_SCLK_PORT, &setup) != SUCCESS) {
        LOG("Failed to configure SPIM SCLK Pin.");
    }
    setup.Pin = SPIM_IO_PIN;
    setup.Alternate = SPIM_IO_AF;
    if (LL_GPIO_Init(SPIM_IO_PORT, &setup) != SUCCESS) {
        LOG("Failed to configure SPIM IO Pin.");
    }

    /*
     * SPIS Pins config
     */
    setup.Pin = SPIS_MOSI_PIN;
    setup.Alternate = SPIS_MOSI_AF;
    if (LL_GPIO_Init(SPIS_MOSI_PORT, &setup) != SUCCESS) {
        LOG("Failed to configure SPIS MOSI Pin.");
    }
    setup.Pin = SPIS_MISO_PIN;
    setup.Alternate = SPIS_MISO_AF;
    if (LL_GPIO_Init(SPIS_MISO_PORT, &setup) != SUCCESS) {
        LOG("Failed to configure SPIS MISO Pin.");
    }
    setup.Pin = SPIS_SCLK_PIN;
    setup.Alternate = SPIS_SCLK_AF;
    if (LL_GPIO_Init(SPIS_SCLK_PORT, &setup) != SUCCESS) {
        LOG("Failed to configure SPIS SCLK Pin.");
    }

    /*
     * Finally, init the peripherals :
     */
    // SPI Master
    LL_SPI_InitTypeDef spim_config = {};

    spim_config.TransferDirection = LL_SPI_HALF_DUPLEX_TX;
    spim_config.Mode = LL_SPI_MODE_MASTER;
    spim_config.DataWidth = LL_SPI_DATAWIDTH_8BIT; // Will be ajusted in any case !
    spim_config.ClockPolarity = LL_SPI_POLARITY_LOW;
    spim_config.ClockPhase = LL_SPI_PHASE_1EDGE;
    spim_config.NSS = LL_SPI_NSS_SOFT;
    spim_config.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV8; // 31.25 MHz
    spim_config.BitOrder = LL_SPI_MSB_FIRST;
    spim_config.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;

    if (LL_SPI_Init(SPIM_SPI_HANDLE, &spim_config) != SUCCESS) {
        LOG("Failed to init the SPIM peripheral");
    }

    LL_SPI_SetFIFOThreshold(SPIM_SPI_HANDLE, LL_SPI_FIFO_TH_01DATA);
    LL_SPI_Enable(SPIM_SPI_HANDLE);

    // SPI Slave
    LL_SPI_InitTypeDef spis_config = {};

    spis_config.TransferDirection = LL_SPI_FULL_DUPLEX;
    spis_config.Mode = LL_SPI_MODE_SLAVE;
    spis_config.DataWidth = LL_SPI_DATAWIDTH_8BIT; // Will be ajusted in any case !
    spis_config.ClockPolarity = LL_SPI_POLARITY_LOW;
    spis_config.ClockPhase = LL_SPI_PHASE_1EDGE;
    spis_config.NSS = LL_SPI_NSS_SOFT;
    spis_config.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV8; // 31.25 MHz
    spis_config.BitOrder = LL_SPI_MSB_FIRST;
    spis_config.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;

    if (LL_SPI_Init(SPIS_SPI_HANDLE, &spis_config) != SUCCESS) {
        LOG("Failed to init the SPIS peripheral");
    }

    LL_SPI_SetFIFOThreshold(SPIS_SPI_HANDLE, LL_SPI_FIFO_TH_01DATA);
    LL_SPI_Enable(SPIS_SPI_HANDLE);
}

void ProbeController::deinitAsJTAG() {

    /*
     * First, remove the peripherals pins
     */
    // Then, reset the pin
    LL_GPIO_InitTypeDef setup = {};
    setup.Pull = LL_GPIO_PULL_NO;
    setup.Mode = LL_GPIO_MODE_ANALOG;

    setup.Pin = SPIM_SCLK_PIN;
    if (LL_GPIO_Init(SPIM_SCLK_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset SPIM SCLK Pin.");
    }
    setup.Pin = SPIM_IO_PIN;
    if (LL_GPIO_Init(SPIM_IO_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset SPIM IO Pin.");
    }
    setup.Pin = SPIS_SCLK_PIN;
    if (LL_GPIO_Init(SPIS_SCLK_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset SPIS SCLK Pin.");
    }
    setup.Pin = SPIS_MOSI_PIN;
    if (LL_GPIO_Init(SPIS_MOSI_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset SPIS MOSI Pin.");
    }
    setup.Pin = SPIS_MISO_PIN;
    if (LL_GPIO_Init(SPIS_MISO_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset SPIS MOSI Pin.");
    }

    /*
     * Finally, remove the clocks
     */
    SPIM_CLOCK_DISABLE
    SPIS_CLOCK_DISABLE

    /*
     * Disable the peripheral
     */
    LL_SPI_Disable(SPIM_SPI_HANDLE);
    LL_SPI_Disable(SPIS_SPI_HANDLE);
}

/* ----------------------------------------------------------------------
 * SPI
 * ----------------------------------------------------------------------
 */
void ProbeController::initAsSPI() {

    /*
     * Enable the clock
     */
    SPIS_CLOCK_ENABLE

    /*
     * Wire the pins
     */
    LL_GPIO_InitTypeDef setup = {};
    setup.Mode = LL_GPIO_MODE_ALTERNATE;
    setup.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    setup.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    setup.Pull = LL_GPIO_PULL_NO;

    setup.Pin = SPIS_MOSI_PIN;
    setup.Alternate = SPIS_MOSI_AF;
    if (LL_GPIO_Init(SPIS_MOSI_PORT, &setup) != SUCCESS) {
        LOG("Failed to configure SPIS MOSI Pin.");
    }
    setup.Pin = SPIS_MISO_PIN;
    setup.Alternate = SPIS_MISO_AF;
    if (LL_GPIO_Init(SPIS_MISO_PORT, &setup) != SUCCESS) {
        LOG("Failed to configure SPIS MISO Pin.");
    }
    setup.Pin = SPIS_SCLK_PIN;
    setup.Alternate = SPIS_SCLK_AF;
    if (LL_GPIO_Init(SPIS_SCLK_PORT, &setup) != SUCCESS) {
        LOG("Failed to configure SPIS SCLK Pin.");
    }

    /*
     * Init the peripheral. We're using the SPIS peripheral as it'll provide a standard SPI access, much easier than
     * combining both.
     */
    LL_SPI_InitTypeDef spis_config = {};
    spis_config.TransferDirection = LL_SPI_FULL_DUPLEX;
    spis_config.Mode = LL_SPI_MODE_MASTER;
    spis_config.DataWidth = LL_SPI_DATAWIDTH_8BIT; // Will be ajusted in any case !
    spis_config.ClockPolarity = LL_SPI_POLARITY_LOW;
    spis_config.ClockPhase = LL_SPI_PHASE_1EDGE;
    spis_config.NSS = LL_SPI_NSS_SOFT;
    spis_config.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV8; // 31.25 MHz
    spis_config.BitOrder = LL_SPI_MSB_FIRST;
    spis_config.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;

    if (LL_SPI_Init(SPIS_SPI_HANDLE, &spis_config) != SUCCESS) {
        LOG("Failed to init the SPIS peripheral");
    }

    LL_SPI_SetFIFOThreshold(SPIS_SPI_HANDLE, LL_SPI_FIFO_TH_01DATA);
    LL_SPI_Enable(SPIS_SPI_HANDLE);
}

void ProbeController::deinitAsSPI() {

    // Then, reset the pin
    LL_GPIO_InitTypeDef setup = {};
    setup.Pull = LL_GPIO_PULL_NO;
    setup.Mode = LL_GPIO_MODE_ANALOG;

    setup.Pin = SPIS_SCLK_PIN;
    if (LL_GPIO_Init(SPIS_SCLK_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset SPIS SCLK Pin.");
    }
    setup.Pin = SPIS_MOSI_PIN;
    if (LL_GPIO_Init(SPIS_MOSI_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset SPIS MOSI Pin.");
    }
    setup.Pin = SPIM_SCLK_PIN;
    if (LL_GPIO_Init(SPIS_MISO_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset SPIS MOSI Pin.");
    }

    /*
     * Disable the clock
     */
    SPIS_CLOCK_DISABLE

    /*
     * Disable the peripheral
     */
    LL_SPI_Disable(SPIS_SPI_HANDLE);
}

/* ----------------------------------------------------------------------
 * GPIO
 * ----------------------------------------------------------------------
 */
void ProbeController::initGPIO() {

    /*
     * Inputs
     */
    LL_GPIO_InitTypeDef setup = {};
    setup.Mode = LL_GPIO_MODE_INPUT;
    setup.Pull = LL_GPIO_PULL_UP;

    setup.Pin = DETECT_PIN;
    if (LL_GPIO_Init(DETECT_PORT, &setup) != SUCCESS) {
        LOG("Failed to configure the DETECT pin");
    }
    setup.Pin = KEY_PIN;
    if (LL_GPIO_Init(KEY_PORT, &setup) != SUCCESS) {
        LOG("Failed to configure the KEY pin");
    }

    /*
     * Outputs
     */
    setup.Mode = LL_GPIO_MODE_OUTPUT;
    setup.Pull = LL_GPIO_PULL_UP;
    setup.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    setup.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;

    setup.Pin = RESET_PIN;
    if (LL_GPIO_Init(RESET_PORT, &setup) != SUCCESS) {
        LOG("Failed to configure the RESET pin");
    }

    /*
     * Configure the default states
     */
    LL_GPIO_SetOutputPin(RESET_PORT, RESET_PIN);

    return;
}

void ProbeController::deinitGPIO() {

    /*
     * Just clear the GPIO to analog :
     */
    LL_GPIO_InitTypeDef setup = {};
    setup.Pull = LL_GPIO_PULL_NO;
    setup.Mode = LL_GPIO_MODE_ANALOG;

    setup.Pin = RESET_PIN;
    if (LL_GPIO_Init(RESET_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset RESET Pin.");
    }
    setup.Pin = KEY_PIN;
    if (LL_GPIO_Init(KEY_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset KEY Pin.");
    }
    setup.Pin = DETECT_PIN;
    if (LL_GPIO_Init(DETECT_PORT, &setup) != SUCCESS) {
        LOG("Failed to reset DETECT Pin.");
    }

    return;
}

/* ----------------------------------------------------------------------
 * GPDMA
 * ----------------------------------------------------------------------
 */
void ProbeController::initGPDMA() {

    LL_DMA_InitTypeDef dma_init = {};
    dma_init.Request = LL_GPDMA1_REQUEST_SPI1_TX; // To update
    dma_init.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    dma_init.SrcIncMode = LL_DMA_SRC_INCREMENT;
    dma_init.DestIncMode = LL_DMA_DEST_FIXED;
    dma_init.SrcDataWidth = LL_DMA_SRC_DATAWIDTH_BYTE;
    dma_init.DestDataWidth = LL_DMA_DEST_DATAWIDTH_BYTE;
    dma_init.Priority = LL_DMA_LOW_PRIORITY_LOW_WEIGHT;
    dma_init.Mode = LL_DMA_NORMAL;
    dma_init.BlkHWRequest = LL_DMA_HWREQUEST_SINGLEBURST;
    LL_DMA_Init(GPDMA1, SPIM_IO_DMA_CHANNEL, &dma_init);
}

void ProbeController::deinitGPDMA() {}

/* ----------------------------------------------------------------------
 * UTILITY
 * ----------------------------------------------------------------------
 */
void ProbeController::flushBuses() {
    LL_SPI_Disable(SPIM_SPI_HANDLE);
    LL_SPI_Disable(SPIS_SPI_HANDLE);

    LL_SPI_ClearFlag_OVR(SPIM_SPI_HANDLE);
    LL_SPI_ClearFlag_UDR(SPIM_SPI_HANDLE);
    LL_SPI_ClearFlag_OVR(SPIS_SPI_HANDLE);
    LL_SPI_ClearFlag_UDR(SPIS_SPI_HANDLE);

    LL_SPI_Enable(SPIM_SPI_HANDLE);
    LL_SPI_Enable(SPIS_SPI_HANDLE);

    return;
}