#ifndef LED_INDICATORS_H
    #define LED_INDICATORS_H

    #include <stdint.h>
    #include <stdbool.h>
    #include "../../core/config/pin_config.h"

    /**
    * @brief Initialize LED indicator GPIOs
    * @return true on success, false on failure
    */
    void led_indicators_init(void);

    /**
    * @brief Turn on HV measured LED
    */
    void led_hv_measured_on(void);

    /**
    * @brief Turn off HV measured LED
    */
    void led_hv_measured_off(void);

    /**
    * @brief Turn on HT measured LED
    */
    void led_ht_measured_on(void);

    /**
    * @brief Turn off HT measured LED
    */
    void led_ht_measured_off(void);

#endif