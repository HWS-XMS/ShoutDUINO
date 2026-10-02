/**
 * @file timer_lib.h
 * @brief Cross-platform timer library for precise timing measurements
 * @details Provides hardware timer frequency detection and counter access
 *          for Arduino Uno Classic (ATmega328P) and R4 variants (Renesas RA4M1)
 */

#ifndef TIMER_LIB_H
#define TIMER_LIB_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Platform Detection
// =============================================================================

#ifdef ARDUINO_AVR_UNO
    #define PLATFORM_ATMEGA328P
#elif defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
    #define PLATFORM_RENESAS_RA4M1
#else
    #warning "Unknown Arduino platform - timing may not be accurate"
#endif

// =============================================================================
// Timer Library Constants
// =============================================================================

#define TIMER_FREQ_MEASURE_DURATION_US  1000    // Duration for frequency measurement
#define TIMER_FREQ_INVALID              0       // Invalid frequency indicator

// =============================================================================
// Timer Library API
// =============================================================================

/**
 * @brief Initialize timer library
 * @return true on success, false on failure
 */
bool timer_lib_init(void);

/**
 * @brief Measure the actual timer frequency using hardware counters
 * @return Timer frequency in Hz, or TIMER_FREQ_INVALID on error
 */
uint32_t timer_lib_measure_frequency(void);

/**
 * @brief Measure timer frequency using hardware frequency counter (preferred)
 * @return Timer frequency in Hz, or TIMER_FREQ_INVALID on error
 */
uint32_t timer_lib_measure_frequency_hardware(void);

/**
 * @brief Get cached timer frequency (measured during init)
 * @return Timer frequency in Hz, or TIMER_FREQ_INVALID if not initialized
 */
uint32_t timer_lib_get_frequency(void);

/**
 * @brief Read current timer counter value
 * @return Current timer counter value
 */
uint32_t timer_lib_read_counter(void);

/**
 * @brief Calculate nanoseconds per timer tick
 * @return Nanoseconds per tick, or 0 if frequency not available
 */
uint32_t timer_lib_get_ns_per_tick(void);

/**
 * @brief Convert nanoseconds to timer ticks
 * @param ns_duration Duration in nanoseconds
 * @return Number of timer ticks for the specified duration
 */
uint32_t timer_lib_ns_to_ticks(uint32_t ns_duration);

/**
 * @brief Convert timer ticks to nanoseconds
 * @param ticks Number of timer ticks
 * @return Duration in nanoseconds
 */
uint32_t timer_lib_ticks_to_ns(uint32_t ticks);

/**
 * @brief Get platform information string
 * @return Platform description string
 */
const char* timer_lib_get_platform_info(void);

/**
 * @brief Get maximum timer resolution in nanoseconds
 * @return Minimum timing resolution achievable
 */
uint32_t timer_lib_get_max_resolution_ns(void);

#endif // TIMER_LIB_H