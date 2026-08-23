/**
 * @file    ShellStreamCdc.hpp
 * @author  lheywang (leonard.heywang@proton.me)
 * @brief   Class to handle a USB CDC Shell interface.
 * @version 0.1
 * @date    2026-03-06
 *
 * @copyright Copyright (c) 2026
 *
 */

// ======================================================================
//                              INCLUDE
// ======================================================================
// Base class
#include "ShellStreamBase.hpp"

// RTOS
#include "ux_api.h"
#include "ux_device_class_cdc_acm.h"

// ======================================================================
//                              CLASS
// ======================================================================
class ShellStreamCdc : public ShellStreamBase {
  private:
    UX_SLAVE_CLASS_CDC_ACM *cdc_instance;
    bool welcome_pending = false;

  protected:
    void hw_transmit(const char *data, const size_t length) override;

  public:
    ShellStreamCdc(char *static_buffer, size_t buffer_size)
        : ShellStreamBase(static_buffer, buffer_size, (const char *)"\r\n", true) {}

    void connect(UX_SLAVE_CLASS_CDC_ACM *cdc);
    void disconnect();

    /*
     * Local override of the process function to ensure we're catching the right moment to send our bannier.
     */
    void process(const char *c, const size_t len) override;
    void process(const char *str) override;
};