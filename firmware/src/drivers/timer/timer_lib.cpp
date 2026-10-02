/**
 * @file timer_lib.cpp
 * @brief Cross-platform timer library implementation
 */

#include "timer_lib.h"
#include <Arduino.h>

// =============================================================================
// Private Variables
// =============================================================================

static uint32_t cached_timer_frequency = TIMER_FREQ_INVALID;
static uint32_t cached_ns_per_tick = 0;
static bool initialized = false;

// =============================================================================
// Platform-Specific Timer Access
// =============================================================================

#ifdef PLATFORM_ATMEGA328P
static uint32_t read_platform_counter(void)
{
    // Use Timer1 (16-bit) on ATmega328P
    return (uint32_t)TCNT1;
}

static void init_platform_timer(void)
{
    // Configure Timer1 for maximum frequency (no prescaling)
    // Normal mode, no compare match, no PWM
    TCCR1A = 0;  // Normal mode
    TCCR1B = (1 << CS10);  // No prescaling (clk/1)
    TCNT1 = 0;   // Clear counter
}

static uint32_t measure_frequency_hardware_atmega328p(void)
{
    // Use Input Capture Unit to measure Timer1 frequency
    // Configure ICP1 (Pin 8) to capture internal clock events
    
    // Save current timer configuration
    uint8_t tccr1a_backup = TCCR1A;
    uint8_t tccr1b_backup = TCCR1B;
    
    // Configure Timer1 for input capture mode
    TCCR1A = 0;  // Normal mode
    TCCR1B = (1 << ICNC1) | (1 << ICES1) | (1 << CS10);  // Noise canceler, rising edge, no prescaling
    TCNT1 = 0;   // Clear counter
    TIFR1 |= (1 << ICF1);  // Clear input capture flag
    
    // Route system clock to input capture (platform specific)
    // For ATmega328P, we need to use an external connection or
    // measure against a known reference frequency
    
    // Alternative: Count timer increments over a known time period
    uint32_t start_micros = micros();
    uint32_t start_count = TCNT1;
    
    // Wait exactly 1000 microseconds
    while ((micros() - start_micros) < 1000) {
        // Wait for precise 1ms
    }
    
    uint32_t end_count = TCNT1;
    uint32_t elapsed_micros = micros() - start_micros;
    
    // Restore timer configuration
    TCCR1A = tccr1a_backup;
    TCCR1B = tccr1b_backup;
    
    // Calculate frequency: (counts / time_us) * 1,000,000
    uint32_t frequency = ((end_count - start_count) * 1000000UL) / elapsed_micros;
    
    return frequency;
}

#elif defined(PLATFORM_RENESAS_RA4M1)
// RA4M1 runs at 48MHz. Use SysTick or GPT for precise timing.
// SysTick is a 24-bit down counter clocked at CPU frequency.

static volatile uint32_t* const SYST_CSR = (volatile uint32_t*)0xE000E010;  // Control/Status
static volatile uint32_t* const SYST_RVR = (volatile uint32_t*)0xE000E014;  // Reload Value
static volatile uint32_t* const SYST_CVR = (volatile uint32_t*)0xE000E018;  // Current Value

static uint32_t read_platform_counter(void)
{
    // SysTick counts DOWN, so invert for consistent behavior
    // Returns a monotonically increasing value
    return (0xFFFFFF - (*SYST_CVR & 0xFFFFFF));
}

static void init_platform_timer(void)
{
    // Configure SysTick for maximum count (free-running)
    *SYST_RVR = 0xFFFFFF;  // Max reload value (24-bit)
    *SYST_CVR = 0;          // Clear current value
    *SYST_CSR = 0x05;       // Enable, use processor clock, no interrupt
}

static uint32_t measure_frequency_hardware_ra4m1(void)
{
    // RA4M1 SysTick runs at CPU clock = 48MHz
    // We can verify by measuring against micros()

    uint32_t start_micros = micros();
    uint32_t start_count = read_platform_counter();

    // Wait for measurement period
    delay(10);  // 10ms for accurate measurement

    uint32_t end_count = read_platform_counter();
    uint32_t end_micros = micros();

    uint32_t elapsed_micros = end_micros - start_micros;

    // Handle 24-bit counter wrap
    uint32_t counter_diff;
    if (end_count >= start_count) {
        counter_diff = end_count - start_count;
    } else {
        counter_diff = (0xFFFFFF - start_count) + end_count + 1;
    }

    // Calculate frequency
    uint64_t frequency = ((uint64_t)counter_diff * 1000000UL) / elapsed_micros;

    return (uint32_t)frequency;
}

#else
// Generic fallback implementation
static uint32_t read_platform_counter(void)
{
    return micros();  // Microsecond resolution fallback
}

static void init_platform_timer(void)
{
    // No platform-specific initialization needed
}
#endif

// =============================================================================
// Public API Implementation
// =============================================================================

bool timer_lib_init(void)
{
    if (initialized) {
        return true;
    }
    
    // Initialize platform-specific timer
    init_platform_timer();
    
    // Small delay to ensure timer stabilization
    delay(1);
    
    // Measure actual timer frequency using hardware counters
    cached_timer_frequency = timer_lib_measure_frequency_hardware();
    
    if (cached_timer_frequency == TIMER_FREQ_INVALID) {
        return false;
    }
    
    // Calculate nanoseconds per tick
    // ns_per_tick = 1,000,000,000 / frequency
    cached_ns_per_tick = 1000000000UL / cached_timer_frequency;
    
    initialized = true;
    return true;
}

uint32_t timer_lib_measure_frequency_hardware(void)
{
#ifdef PLATFORM_ATMEGA328P
    return measure_frequency_hardware_atmega328p();
#elif defined(PLATFORM_RENESAS_RA4M1)
    return measure_frequency_hardware_ra4m1();
#else
    // Fallback to timing-based measurement
    return timer_lib_measure_frequency();
#endif
}

uint32_t timer_lib_measure_frequency(void)
{
    // Fallback timing-based measurement for compatibility
    uint32_t start_micros = micros();
    uint32_t start_counter = read_platform_counter();
    
    // Wait for measurement period
    delayMicroseconds(TIMER_FREQ_MEASURE_DURATION_US);
    
    // Capture end conditions
    uint32_t end_micros = micros();
    uint32_t end_counter = read_platform_counter();
    
    // Calculate differences with wrap-around handling
    uint32_t elapsed_micros = end_micros - start_micros;
    
    // Handle timer counter wrap-around
    uint32_t counter_diff;
    if (end_counter >= start_counter) {
        counter_diff = end_counter - start_counter;
    } else {
        // Timer wrapped around: calculate difference across the wrap boundary
        counter_diff = (UINT32_MAX - start_counter) + end_counter + 1;
    }
    
    // Validate measurement
    if (elapsed_micros == 0 || counter_diff == 0) {
        return TIMER_FREQ_INVALID;
    }
    
    // Calculate frequency using 64-bit arithmetic to prevent overflow
    // For high-frequency timers (R4 @ 48MHz), counter_diff * 1,000,000 can exceed uint32_t
    uint64_t frequency_64 = ((uint64_t)counter_diff * 1000000UL) / elapsed_micros;
    
    // Clamp to uint32_t range
    uint32_t frequency = (frequency_64 > UINT32_MAX) ? UINT32_MAX : (uint32_t)frequency_64;
    
    return frequency;
}

uint32_t timer_lib_get_frequency(void)
{
    return cached_timer_frequency;
}

uint32_t timer_lib_read_counter(void)
{
    return read_platform_counter();
}

uint32_t timer_lib_get_ns_per_tick(void)
{
    return cached_ns_per_tick;
}

uint32_t timer_lib_ns_to_ticks(uint32_t ns_duration)
{
    if (cached_ns_per_tick == 0) {
        return 0;
    }
    
    return (ns_duration + cached_ns_per_tick - 1) / cached_ns_per_tick;  // Round up
}

uint32_t timer_lib_ticks_to_ns(uint32_t ticks)
{
    return ticks * cached_ns_per_tick;
}

const char* timer_lib_get_platform_info(void)
{
#ifdef PLATFORM_ATMEGA328P
    return "ATmega328P (Arduino Uno Classic/R3)";
#elif defined(PLATFORM_RENESAS_RA4M1)
    return "Renesas RA4M1 (Arduino Uno R4)";
#else
    return "Unknown Platform";
#endif
}

uint32_t timer_lib_get_max_resolution_ns(void)
{
    return cached_ns_per_tick;
}