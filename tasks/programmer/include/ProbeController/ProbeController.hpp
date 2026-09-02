/**
 * @file    ProbeController.hpp
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Define the probe low level actions, used to by any probe classes.
 *
 * @details This probe controller is based over two SPI, respectively SPIM and SPIS.
 *          SPIM is used as a half duplex SPI Master, and SPIS is used as full duplex SPI Slave.
 *          - SPIM : Handle the clock output (SWCLK/TCK) and the data output (SWDIO/TMS).
 *          - SPIS : Handle the pure data input (SWO/TDO) and output (-/TDI).
 *
 *          The two clocks are physically tied together to ensure the lowest difference as possible.
 *
 *          On the latest revisions, SPIS can also be used as UART RX for the SWD SWO output, which is
 *          asynchronous.
 *
 * @version 0.1
 * @date    2026-08-31
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

// ======================================================================
//                               INCLUDES
// ======================================================================
// Other headers
#include "ProbeConfig.hpp"
#include "ProgrammerCommands.h"

// STD
#include <cstdint>
#include <span>

// ======================================================================
//                               VARIABLES
// ======================================================================
extern ProbeBoardConfig configV1;
extern TX_SEMAPHORE programmer_dma;

// ======================================================================
//                               CLASS
// ======================================================================
/**
 * @brief Implement the low level IO procedures to be done on the hardware board.
 *        Protocols are going to exploit these primitives.
 *
 */
class ProbeController {

  private:
    TargetProtocol mode; /**< The mode to be used. Ensure no hardware conflicts on this level.*/

    /*
     * Utility functions to change the peripheral configuration during the execution.
     */
    void initAsSWD();
    void deinitAsSWD();
    void initAsSWDO();
    void deinitAsSWDO();
    void initAsJTAG();
    void deinitAsJTAG();
    void initAsSPI();
    void deinitAsSPI();

    /*
     * Utility functions to initalize the GPIO
     */
    void initGPIO();
    void deinitGPIO();

    /*
     * Utility to initialize the DMA engine
     */
    void initGPDMA();
    void deinitGPDMA();

    /*
     * Bus management
     */
    void flushBuses();

  public:
    /*
     * C++ API
     */
    ProbeController();
    ~ProbeController();

    /*
     * PROBE API
     */
    /**
     * @brief Set the DUT power state.
     *
     * @warning This option effect may depends on the used extension board. The default one ignore this parameter.
     *
     * @param[in] enable The state of this bool reflect to the power status of the board.
     *
     * @return true The power was configured and the board does support this feature.
     * @return false The power was not configured or the board does not support this feature.
     */
    bool setTargetPower(const bool enable);

    /**
     * @brief Read the DUT applied voltage.
     *
     * @return float The voltage of the DUT, in volts.
     */
    float getTargetVoltage();

    /**
     * @brief Configure the DUT reset state.
     *
     * @param[in] status The state of this pin reflect to the reset pin of the connector.
     *
     * @return true The reset was configured.
     */
    bool setResetPin(const bool status);

    /**
     * @brief Configure the mode of the probe.
     *
     * @warning No mutex checks are done. This is only used as a guard for incorrect calls, especially on the
     *          turnardound procedures.
     *
     * @param[in] request The requested mode of operation.
     * @return true The mode was configured.
     */
    bool configureMode(const TargetProtocol request);

    /**
     * @brief Set the bus frequency of the probe. The value is passed in Hz.
     *
     * @param[in] frequency The requested frequency.
     *
     * @return uint32_t
     * @retval 0 The bus frequency was incorrect. Consider this return value as an error.
     * @retval >0 The configured bus frequency. The value may not match exactly the request, due to hardware
     *            limitations.
     */
    uint32_t setBusFrequency(const uint32_t frequency);

    /**
     * @brief Toggle the direction of the bus. This function is only enabled on SWD based modes.
     *
     * @return true The bus direction was toggled.
     * @return false The bus direction was not toggled (ie, the probe is configured in a wrong mode).
     */
    bool toggleDirection();

    /**
     * @brief Shift the bits on the selected bus.
     *
     * @param[in] tms The bit vector to be shifted out over the TMS/SWDIO bus.
     * @param[in] tdi The bit vector to be shifted out over the TDI bus.
     * @param[out] tdo The bit vector to be shifted in over the TDO bus.
     * @param[in] bitCount The number of **BITS** to be shifted. This length must therefore be ~8x bigger than the
     *                     length of the passed vectors.
     *
     * @par Limitations
     * @warning This function is designed to exploit the latest ST's hardware options. The hardware impose a minimal
     *          transfer size of 4 bits. The protocol handler must account for that limitation.
     *          There is no maximum transfer size.
     *
     * @par Operation
     * @warning For smallest transfers where the polling is used, the function may disable the interrupts, and therefore
     *          the RTOS task scheduling.
     *
     * @return true The transfer was completed.
     * @return false The transfer was not completed and / or the size was too small (< 4 bits).
     */
    bool ioBits(std::span<const uint8_t> tms, std::span<const uint8_t> tdi, std::span<uint8_t> tdo, size_t bitCount);

    /**
     * @brief Wait for a defined amout of time. The timing is required to be at least the length, but could not be
     * guaranteed due to the underlying RTOS layers.
     *
     * @param[in] us The provided duration in us.
     * @return true The delay was elapsed.
     */
    bool delayus(const uint32_t us);
};