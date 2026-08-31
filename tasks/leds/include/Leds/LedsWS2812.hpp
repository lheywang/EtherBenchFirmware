/**
 * @file    LedsWs2812.hpp
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Define a basic WS2812 controller class.
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
//                              INCLUDES
// ======================================================================
// Local libraries
#include "Leds/LedsBaseCommands.h"
#include "LedsBase.hpp"

// HAL
#include "stm32h5xx_hal.h"

// STD
#include <cstddef>
#include <cstdint>

// ======================================================================
//                              DEFINES
// ======================================================================
// -------------------------------------
#define WS2812_BIT_NS 1250 //  1.25 us

// 0
#define WS2812_T0H_NS 350
#define WS2812_T0L_NS 800

// 1
#define WS2812_T1H_NS 700
#define WS2812_T1L_NS 600

// RES
#define WS2812_RES_NS 50000

// -------------------------------------
// Effects parameters
#define WS2812_FLASH_PERIOD_MS     LEDS_T_Hz(5)
#define WS2812_SPIN_PERIOD_MS      LEDS_T_Hz(2)
#define WS2812_BREATHING_PERIOD_MS LEDS_T_Hz(30)

#define WS2812_VU_METER_PERIOD_MS  LEDS_T_Hz(50)
#define WS2812_RAINBOW_PERIOD_MS   LEDS_T_Hz(20)
#define WS2812_HEARTBEAT_PERIOD_MS LEDS_T_Hz(5)

/* Actually unused, we just stop the timer*/
#define WS2812_SOLID_PERIOD_MS    LEDS_T_Hz(1)
#define WS2812_PROGRESS_PERIOD_MS LEDS_T_Hz(1)
// ======================================================================
//                              STRUCTS
// ======================================================================

struct PixelEffect {
    leds_effects type; ///< Effect type
    Pixel primary;     ///< Main color effect.
    Pixel secondary;   ///< Main color effect.
    uint16_t speed;    ///< Effect speed (delay of multiplier)
    uint8_t width;     ///< Effect width (tail length)
    uint8_t progress;  ///< Effect progress, for the current value.
    uint16_t tick;     ///< Internal tick counter, for advanced effects.
};

// ======================================================================
//                              CLASS
// ======================================================================

class LedsWS2812 : public LedsBase {

  private:
    // Raw led buffer
    uint32_t __aligned(32) leds_buffer[(LED_RING_BIT_PER_PIXEL * LED_RING_PIXEL_NB) + 64];
    // Pixel buffer
    Pixel pixel_buffer[LED_RING_PIXEL_NB];

    // Timer handle for the output DMA
    TIM_HandleTypeDef *htim;
    uint32_t timer_channel;
    uint32_t timer_freq;

    // Internal structure for the effects.
    PixelEffect current_effect;
    leds_effects previous_effect;

    // Internal timer configs
    uint16_t T0H;
    uint16_t T1H;

    /**
     * @brief Configure the HW timer to match the requirements.
     *        Freq is in kHz
     *
     */
    void init_hw_timer();
    void stop_hw_timer();

    /*
     * All the following are just effects functions
     */
    void effect_flash();
    void effect_solid();
    void effect_spin();
    void effect_breathing();
    void effect_progress();
    void effect_vu_meter();
    void effect_rainbow();
    void effect_heartbeat();

    inline Pixel apply_alpha(Pixel *input, uint8_t alpha);

    /**
     * @brief Trigger the DMA refresh sequence.
     *
     */
    void refresh_leds();

    void compute_timer_cycles();
    void send_buffer();

  protected:
    void on_timer_tick(ULONG arg) override;

  public:
    LedsWS2812(TX_TIMER *timer, TIM_HandleTypeDef *htim, uint32_t timer_channel, uint32_t TimerFreq);
    LedsWS2812(TX_TIMER *timer,
               const char *timer_name,
               TIM_HandleTypeDef *htim,
               uint32_t timer_channel,
               uint32_t TimerFreq);

    ~LedsWS2812();

    /**
     * @brief Initialize the hardware and the software timer.
     *        Enable to auto-refresh the effect without requiring on a precise time base.
     *
     */
    void init();

    /**
     * @brief Set a new effect on the led ring. Can be called from anywhere.
     *
     * @param new_effect The PixelEffect struct to be applied.
     */
    void set_effect(const PixelEffect new_effect);

    /**
     * @brief Update the effect status in the loop.
     *
     * @param progress
     */
    void set_effect_progress(uint8_t progress);
    void set_effect_progress();
};

/**
 * @brief ISR routine to be used to conclude the refresh pulse.
 *
 */
void ISR_EndRefresh();

#ifdef __cplusplus
}
#endif