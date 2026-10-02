#ifndef PULSE_GENERATOR_H
    #define PULSE_GENERATOR_H

    #include <stdint.h>
    #include <stdbool.h>
    #include "../config/pin_config.h"

    #define PULSE_MAX_WIDTH_NS      200     // Maximum pulse width in nanoseconds
    #define PULSE_MIN_WIDTH_NS      10      // Minimum pulse width in nanoseconds

    typedef enum {
        PULSE_RESULT_SUCCESS = 0,           // Pulse generated successfully
        PULSE_RESULT_WIDTH_TOO_LONG,        // Requested width exceeds maximum
        PULSE_RESULT_WIDTH_TOO_SHORT,       // Requested width below minimum
        PULSE_RESULT_TIMER_ERROR,           // Timer system error
        PULSE_RESULT_NOT_INITIALIZED        // Pulse generator not initialized
    } pulse_result_t;

    /**
    * @brief Initialize pulse generator with timer library
    * @return true on success, false on failure
    */
    bool pulse_generator_init(void);

    /**
    * @brief Generate a single pulse with specified width
    * @param width_ns Pulse width in nanoseconds (10-200ns)
    * @return Pulse generation result
    * @warning This function may disable interrupts briefly during pulse generation
    */
    pulse_result_t pulse_generator_pulse(uint32_t width_ns);

    /**
    * @brief Generate a pulse with specified timer ticks
    * @param ticks Number of timer ticks for pulse width
    * @return Pulse generation result
    * @warning This function may disable interrupts briefly during pulse generation
    */
    pulse_result_t pulse_generator_pulse_ticks(uint32_t ticks);

    /**
    * @brief Generate the shortest possible pulse
    * @return Pulse generation result
    * @warning This function may disable interrupts briefly during pulse generation
    */
    pulse_result_t pulse_generator_pulse_minimum(void);

    /**
    * @brief Get actual pulse width for a requested nanosecond value
    * @param requested_ns Requested pulse width in nanoseconds
    * @return Actual pulse width that would be generated (quantized to timer resolution)
    */
    uint32_t pulse_generator_get_actual_width_ns(uint32_t requested_ns);

    /**
    * @brief Get maximum achievable resolution
    * @return Minimum pulse width increment in nanoseconds
    */
    uint32_t pulse_generator_get_resolution_ns(void);

    /**
    * @brief Get timer frequency being used
    * @return Timer frequency in Hz
    */
    uint32_t pulse_generator_get_timer_frequency(void);

    /**
    * @brief Get platform information
    * @return Platform description string
    */
    const char* pulse_generator_get_platform_info(void);

#endif