/**
 * @file    LedsWS2812.cpp
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief
 * @version 0.1
 * @date    2026-03-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#define LOG_MODULE "WS2812"

// ======================================================================
//                              INCLUDES
// ======================================================================
// Header
#include "Leds/LedsWS2812.hpp"

// Local libraries
#include "Leds/LedsBase.hpp"
#include "Leds/LedsBaseCommands.h"
#include "logger.h"

// LUT correction table
#include "gamma_lut.h"

// HAL
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_tim.h"

// STD
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <math.h>

// RTOS
#include "tx_api.h"
#include "tx_handler.h"

// ======================================================================
//                              EXTERNS
// ======================================================================

extern DCACHE_HandleTypeDef hdcache1;

// ======================================================================
//                              CONSTANTS
// ======================================================================

constexpr size_t HEARTBEAT_SAMPLES = 100;
constexpr std::array<uint8_t, HEARTBEAT_SAMPLES> generate_heartbeat_lut() {

    std::array<uint8_t, HEARTBEAT_SAMPLES> table{};

    for (size_t phase = 0; phase < HEARTBEAT_SAMPLES; phase += 1) {
        if (phase < 15) {
            table[phase] = (phase * 255) / 15;
        } else if (phase < 30) {
            table[phase] = 255 - ((phase - 15) * 255) / 15;
        }

        else if (phase >= 40 && phase < 50) {
            table[phase] = ((phase - 40) * 200) / 10;
        } else if (phase >= 50 && phase < 60) {
            table[phase] = 200 - ((phase - 50) * 200) / 10;
        }
    }

    return table;
}

constexpr auto heartbeat_lut = generate_heartbeat_lut();

// ======================================================================
//                              PRIVATES
// ======================================================================
ULONG getThreadXTimerCounts(uint32_t ms) {

    if (ms < 10) {
        return 1;
    } else {
        return (ms / 10) + 1;
    }
}

// ======================================================================
//                              CLASS IMPL.
// ======================================================================
LedsWS2812::LedsWS2812(TX_TIMER *timer, TIM_HandleTypeDef *htim, uint32_t timer_channel, uint32_t TimerFreq)
    : LedsBase(timer) {

    // Copy new values
    this->htim = htim;
    this->timer_channel = timer_channel;
    this->timer_freq = TimerFreq;

    return;
}

LedsWS2812::LedsWS2812(TX_TIMER *timer,
                       const char *timer_name,
                       TIM_HandleTypeDef *htim,
                       uint32_t timer_channel,
                       uint32_t TimerFreq)
    : LedsBase(timer) {

    (void)timer_name;

    // Copy new values
    this->htim = htim;
    this->timer_channel = timer_channel;
    this->timer_freq = TimerFreq;
    this->previous_effect = leds_effects::EFFECT_NONE;

    return;
}
LedsWS2812::~LedsWS2812() {
    this->stop_hw_timer();
    // Mother class is destructed automatically.
    return;
}

void LedsWS2812::init() {

    // Init the provided timer.
    ULONG status = this->init_timer("RingLed01");
    if (status != TX_SUCCESS) {
        LOG("Failed to create the timer with error = %d", status);
    };

    // Clear buffers
    memset(&this->leds_buffer, 0x00, sizeof(this->leds_buffer));
    memset(&this->pixel_buffer, 0x00, sizeof(this->pixel_buffer));

    // Init the hardware
    this->init_hw_timer();

    LOG("Initialized the WS2812 engine");

    return;
};

void LedsWS2812::on_timer_tick(ULONG arg) {
    LedsWS2812 *ClassInstance = reinterpret_cast<LedsWS2812 *>(arg);
    ClassInstance->refresh_leds();
    return;
}

void LedsWS2812::init_hw_timer() {

    // Get the period in ns.
    uint32_t period = 1000000000ULL / this->timer_freq;

    // Compute timer options
    this->T0H = WS2812_T0H_NS / period;
    this->T1H = WS2812_T1H_NS / period;

    // Basic timer init
    this->htim->Init.Prescaler = 0;
    this->htim->Init.CounterMode = TIM_COUNTERMODE_UP;

    this->htim->Init.Period = (WS2812_BIT_NS / period);

    this->htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    this->htim->Init.RepetitionCounter = 0;

    this->htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(this->htim) != HAL_OK) {
        Tx_Error_Handler(RING_FAIL_TIM_INIT);
    }

    TIM_OC_InitTypeDef sConfigOC;
    memset(&sConfigOC, 0x00, sizeof(sConfigOC));

    sConfigOC.OCMode = TIM_OCMODE_PWM1;

    sConfigOC.Pulse = 0;

    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCIDLESTATE_RESET;

    if (HAL_TIM_PWM_ConfigChannel(this->htim, &sConfigOC, this->timer_channel) != HAL_OK) {
        Tx_Error_Handler(RING_FAIL_PWM_INIT);
    }

    /*
     * Start the PWM
     */
    HAL_TIM_PWM_Start(this->htim, TIM_CHANNEL_1);

    return;
}

void LedsWS2812::stop_hw_timer() {

    // Stop any DMA :
    HAL_TIM_PWM_Stop_DMA(this->htim, this->timer_channel);

    return;
}

void LedsWS2812::refresh_leds() {
    // Update to say : Hey, I've registerd that change !
    if (this->previous_effect != this->current_effect.type) {
        this->current_effect.tick = 0;
        this->current_effect.progress = 0;
    }
    this->previous_effect = this->current_effect.type;

    /*
     * Call the right update function
     */
    switch (this->current_effect.type) {

    case leds_effects::EFFECT_FLASH:
        this->effect_flash();
        break;

    case leds_effects::EFFECT_SOLID:
        this->effect_solid();
        break;

    case leds_effects::EFFECT_SPIN:
        this->effect_spin();
        break;

    case leds_effects::EFFECT_BREATHING:
        this->effect_breathing();
        break;

    case leds_effects::EFFECT_PROGRESS:
        this->effect_progress();
        break;

    case leds_effects::EFFECT_VU_METER:
        this->effect_vu_meter();
        break;

    case leds_effects::EFFECT_RAINBOW:
        this->effect_rainbow();
        break;

    case leds_effects::EFFECT_HEARTBEAT:
        this->effect_heartbeat();
        break;

    default:
        break;
    }

    // Update the leds
    this->compute_timer_cycles();
    this->send_buffer();
    return;
}

void LedsWS2812::compute_timer_cycles() {

    /*
     * Generate the pixels
     */
    uint32_t dma_index = 0;
    // We fill the buffer, depending on the target value.
    for (int pixel = 0; pixel < LED_RING_PIXEL_NB; pixel += 1) {

        uint32_t grb = ((uint32_t)this->pixel_buffer[pixel].aRGB.g << 16) |
                       ((uint32_t)this->pixel_buffer[pixel].aRGB.r << 8) | ((uint32_t)this->pixel_buffer[pixel].aRGB.b);

        for (int bit = 23; bit >= 0; bit--) {
            this->leds_buffer[dma_index++] = ((grb >> bit) & 0x01) ? this->T1H : this->T0H;
        }
    }

    // Fill the last 64 bytes with 0 to ensure the > 50 us for the color latch
    for (int i = 0; i < 64; i++) {
        this->leds_buffer[dma_index++] = 0;
    }

    return;
}

void LedsWS2812::send_buffer() {

    HAL_StatusTypeDef ret = HAL_OK;

    HAL_TIM_StateTypeDef status = HAL_TIM_PWM_GetState(this->htim);
    if (status != HAL_TIM_STATE_READY) {
        LOG("Cannot send the buffer. An operation is already running...");
        return;
    }

    // Stop any current DMA transfers
    ret = HAL_TIM_PWM_Stop_DMA(this->htim, this->timer_channel);
    if (ret != HAL_OK) {
        LOG("WARNING : Failed to stop the previous DMA session. Status is %d", (int)ret);
    }

    // Flush the caches
    ret = HAL_DCACHE_CleanByAddr(
        &hdcache1, reinterpret_cast<uint32_t *>(this->leds_buffer), (sizeof(this->leds_buffer) + 31) & ~31);
    if (ret != HAL_OK) {
        LOG("WARNING : Failed cleaning the cache for the DMA. Status is %d", (int)ret);
    }

    // Trigger the next serie of DMA
    ret = HAL_TIM_PWM_Start_DMA(
        this->htim, this->timer_channel, reinterpret_cast<uint32_t *>(this->leds_buffer), sizeof(this->leds_buffer));
    if (ret != HAL_OK) {
        LOG("WARNING : Failed to start the new DMA session. Status is %d", (int)ret);
    }

    return;
}

void LedsWS2812::set_effect(const PixelEffect new_effect) {

    // Check if we have something to do ...
    if (new_effect.type == this->previous_effect) {
        return;
    }

    // First, stop the timer (to never refresh while computing)
    this->stop_timer();

    /*
     * Copy the new parameters and for the refresh.
     * This will write to the leds
     */
    memcpy(&this->current_effect, &new_effect, sizeof(PixelEffect));

    /*
     * Restart the timer with the new parameters
     */
    switch (this->current_effect.type) {

    case leds_effects::EFFECT_FLASH:
        this->start_timer(getThreadXTimerCounts(WS2812_FLASH_PERIOD_MS));
        LOG("Configured effect FLASH");
        break;

    case leds_effects::EFFECT_SPIN:
        this->start_timer(getThreadXTimerCounts(WS2812_SPIN_PERIOD_MS));
        LOG("Configured effect SPIN");
        break;

    case leds_effects::EFFECT_BREATHING:
        this->start_timer(getThreadXTimerCounts(WS2812_BREATHING_PERIOD_MS));
        LOG("Configured effect BREATHING");
        break;
    case leds_effects::EFFECT_PROGRESS:
        this->start_timer(getThreadXTimerCounts(WS2812_PROGRESS_PERIOD_MS));
        LOG("Configured effect PROGRESS");
        break;

    case leds_effects::EFFECT_VU_METER:
        // this->start_timer(getThreadXTimerCounts(WS2812_VU_METER_PERIOD_MS));
        LOG("Deprecated mode : VU-METER. No effect where applied.");

        break;

    case leds_effects::EFFECT_RAINBOW:
        this->start_timer(getThreadXTimerCounts(WS2812_RAINBOW_PERIOD_MS));
        LOG("Configured effect RAINBOW");

        break;

    case leds_effects::EFFECT_HEARTBEAT:
        this->start_timer(getThreadXTimerCounts(WS2812_HEARTBEAT_PERIOD_MS));
        LOG("Configured effect HEARTBEAT");

        break;

    default:
    case leds_effects::EFFECT_SOLID:
        break;
    }
    return;
}

void LedsWS2812::set_effect_progress(uint8_t progress) {
    this->current_effect.progress = progress & 0xFF;
    return;
}

void LedsWS2812::set_effect_progress() {
    uint8_t progress = this->current_effect.progress += 1;
    this->set_effect_progress(progress);
    return;
}

void LedsWS2812::effect_flash() {

    // Swap the tick value between 0 and 1
    this->current_effect.tick += 1;

    if (this->current_effect.tick > 10) {
        this->current_effect.tick = 0;
    }

    // Select the target colour
    Pixel *temp = (this->current_effect.tick > 5) ? &this->current_effect.primary : &this->current_effect.secondary;

    // Update the pixels
    for (int pixel = 0; pixel < LED_RING_PIXEL_NB; pixel += 1) {
        this->pixel_buffer[pixel] = this->apply_alpha(temp, gamma_lut[temp->aRGB.alpha]);
    }

    return;
}
void LedsWS2812::effect_solid() {

    // Increment the tick counter (shall be useless anyway, timer is stopped)
    this->current_effect.tick += 1;

    // Update the pixels
    for (int pixel = 0; pixel < LED_RING_PIXEL_NB; pixel += 1) {
        this->pixel_buffer[pixel] =
            this->apply_alpha(&this->current_effect.primary, gamma_lut[this->current_effect.primary.aRGB.alpha]);
    }

    return;
}

void LedsWS2812::effect_breathing() {

    /*
     * This block will make a generate a ramp until LED_RING_BREATH_MAX is reached,
     * Then, a decreasing ramp for the same duration, and, finally, a 0 hold for some
     * cycles until LED_RING_BREATH_LENGTH is reached.
     */

    // Handle the tick counter update
    uint16_t local_tick = 0;
    this->current_effect.tick += 1;

    if (this->current_effect.tick <= LED_RING_BREATH_MAX) {
        local_tick = this->current_effect.tick;

    } else if (this->current_effect.tick <= (2 * LED_RING_BREATH_MAX)) {
        local_tick = (2 * LED_RING_BREATH_MAX) - this->current_effect.tick;

    } else if (this->current_effect.tick < LED_RING_BREATH_LENGTH) {
        local_tick = 0;

    } else {
        this->current_effect.tick = 0;
        local_tick = 0;
    }

    LOG("Local tick = %d", local_tick);

    // Fetch the gamma for that value
    uint8_t Intensity = gamma_lut[(uint8_t)local_tick];

    // Apply to all pixels
    for (int pixel = 0; pixel < LED_RING_PIXEL_NB; pixel += 1) {
        this->pixel_buffer[pixel] = this->apply_alpha(&this->current_effect.primary, Intensity);
    }

    return;
}

void LedsWS2812::effect_spin() {

    // Handle the tick update
    if (this->current_effect.tick < LED_RING_PIXEL_NB) {
        this->current_effect.tick += 1;
    } else {
        this->current_effect.tick = 0;
    }

    // Compute the leds statuses
    uint8_t intensities[LED_RING_PIXEL_NB] = {};

    uint8_t width = this->current_effect.width % LED_RING_PIXEL_NB;
    width = (width == 0) ? 1 : width;
    uint8_t step = LED_CORR_MAXVAL / width;

    uint8_t lum = LED_CORR_MAXVAL;

    for (int tail_pos = 0; tail_pos < LED_RING_PIXEL_NB; tail_pos += 1) {
        intensities[tail_pos] = gamma_lut[lum];

        if (lum > step) {
            lum -= step;
        } else {
            lum = 0;
        }
    }

    /*
     * Until now, the first index is the maximal level index, and the
     * this->current_effect.width is the first to be empty.
     *
     * This array does not take account for the real position !
     */

    // Apply the leds to the effects :
    for (int pixel = 0; pixel < LED_RING_PIXEL_NB; pixel += 1) {
        int8_t tail_index = pixel - static_cast<int8_t>(this->current_effect.tick);
        if (tail_index < 0) {
            tail_index += LED_RING_PIXEL_NB;
        }

        this->pixel_buffer[pixel] = this->apply_alpha(&this->current_effect.primary, intensities[tail_index]);
    }

    return;
}

void LedsWS2812::effect_progress() {

    /*
     * The value of the progress is only configured by the external signals.
     * We limit to 100 to prevent from overflows.
     */
    if (this->current_effect.progress > 100) {
        this->current_effect.progress = 100;
    }

    uint8_t intensities[LED_RING_PIXEL_NB] = {0};

    uint8_t step = 100 / LED_RING_PIXEL_NB;
    uint8_t lum = this->current_effect.progress;

    /*
     * Compute the pixel update
     */
    for (int pixel = 0; pixel < LED_RING_PIXEL_NB; pixel += 1) {
        if (lum > step) {
            intensities[pixel] = gamma_lut[255];
            lum -= step;
        } else if (lum > 0) {
            intensities[pixel] = (lum * LED_CORR_MAXVAL) / step;
            lum = 0;
        } else {
            intensities[pixel] = 0;
        }
    }

    /*
     * Update the luminances.
     */
    for (int pixel = 0; pixel < LED_RING_PIXEL_NB; pixel += 1) {
        this->pixel_buffer[pixel] = this->apply_alpha(&this->current_effect.primary, intensities[pixel]);
    }

    return;
}

void LedsWS2812::effect_vu_meter() {

    // No ticks are updates are needed here. This function is only called on progress
    // updates.

    uint32_t total_brightness = this->current_effect.progress * (LED_RING_PIXEL_NB / 2);

    // Compute the target alpha
    for (int pixel = 0; pixel < (LED_RING_PIXEL_NB / 2); pixel += 1) {
        uint32_t pixel_start = pixel * LED_CORR_MAXVAL;

        /*
         * Select the intensity based on some
         */
        uint8_t intensity = 0;

        if (total_brightness > (pixel_start + LED_CORR_MAXVAL)) {
            intensity = LED_CORR_MAXVAL;
        } else if (total_brightness > pixel_start) {
            intensity = total_brightness - pixel_start;
        }

        // Apply the pixel value
        this->pixel_buffer[(LED_RING_PIXEL_NB / 2) + pixel] =
            this->apply_alpha(&this->current_effect.primary, gamma_lut[intensity]);
        this->pixel_buffer[(LED_RING_PIXEL_NB / 2) - pixel] =
            this->apply_alpha(&this->current_effect.primary, gamma_lut[intensity]);
    }

    return;
}

void LedsWS2812::effect_rainbow() {

    // Increment the tick
    this->current_effect.tick += 1;

    // Get the offset to be applied
    uint8_t offset = (this->current_effect.tick * this->current_effect.speed) & 0xFF;

    // Reset the tick when we hit 0
    if (offset == 0) {
        this->current_effect.tick = 0;
    }

    for (int pixel = 0; pixel < LED_RING_PIXEL_NB; pixel += 1) {

        uint8_t hue = (pixel * 256 / LED_RING_PIXEL_NB) + offset;

        Pixel color = this->current_effect.primary;
        hue = 255 - hue;

        if (hue < 85) {
            color.aRGB.r = 255 - (hue * 3);
            color.aRGB.g = 0;
            color.aRGB.b = (hue * 3);
        } else if (hue < 170) {
            color.aRGB.r = 0;
            color.aRGB.g = (hue * 3);
            color.aRGB.b = 255 - (hue * 3);
        } else {
            color.aRGB.r = (hue * 3);
            color.aRGB.g = 255 - (hue * 3);
            color.aRGB.b = 0;
        }

        this->pixel_buffer[pixel] = this->apply_alpha(&color, gamma_lut[color.aRGB.alpha]);
    }
    return;
}

void LedsWS2812::effect_heartbeat() {

    this->current_effect.tick += 1;
    if (this->current_effect.tick >= HEARTBEAT_SAMPLES) {
        this->current_effect.tick = 0;
    }

    uint8_t raw_intensity = heartbeat_lut[this->current_effect.tick];
    uint8_t intensity = gamma_lut[raw_intensity];

    for (int pixel = 0; pixel < LED_RING_PIXEL_NB; pixel += 1) {
        this->pixel_buffer[pixel] = this->apply_alpha(&this->current_effect.primary, intensity);
    }

    return;
}

inline Pixel LedsWS2812::apply_alpha(Pixel *input, uint8_t alpha) {

    Pixel output;
    memset(&output, 0x00, sizeof(Pixel));

    output.aRGB.r = (uint8_t)(((uint32_t)input->aRGB.r * alpha) / 255);
    output.aRGB.g = (uint8_t)(((uint32_t)input->aRGB.g * alpha) / 255);
    output.aRGB.b = (uint8_t)(((uint32_t)input->aRGB.b * alpha) / 255);
    output.aRGB.alpha = alpha;

    return output;
}
