#include "pulse_generator.h"
#include "../../drivers/timer/timer_lib.h"
#include <Arduino.h>

static void pulse_output_high(void) {
    digitalWrite(PULSE_GENERATOR_PIN, HIGH);
}

static void pulse_output_low(void) {
    digitalWrite(PULSE_GENERATOR_PIN, LOW);
}

static void precise_delay_ticks(uint32_t ticks) {
    if (ticks == 0) {
        return;
    }
    // Get starting counter value
    uint32_t start_counter = timer_lib_read_counter();

    // Busy wait until elapsed ticks reached
    // Handles 24-bit counter wrap-around (SysTick on RA4M1)
    while (true) {
        uint32_t current = timer_lib_read_counter();
        uint32_t elapsed;

        if (current >= start_counter) {
            elapsed = current - start_counter;
        } else {
            // Counter wrapped (24-bit)
            elapsed = (0xFFFFFF - start_counter) + current + 1;
        }

        if (elapsed >= ticks) {
            break;
        }
        __asm__ __volatile__ ("nop");
    }
}

bool pulse_generator_init(void) {
    pinMode(PULSE_GENERATOR_PIN, OUTPUT);
    digitalWrite(PULSE_GENERATOR_PIN, LOW);
    if (timer_lib_get_frequency() == TIMER_FREQ_INVALID) {
        return false;
    }
    return true;
}

pulse_result_t pulse_generator_pulse(uint32_t width_ns) {
    if (width_ns > PULSE_MAX_WIDTH_NS) {
        return PULSE_RESULT_WIDTH_TOO_LONG;
    }
    if (width_ns < PULSE_MIN_WIDTH_NS) {
        return PULSE_RESULT_WIDTH_TOO_SHORT;
    }
    uint32_t ticks = timer_lib_ns_to_ticks(width_ns);
    if (ticks == 0) {
        return PULSE_RESULT_TIMER_ERROR;
    }
    return pulse_generator_pulse_ticks(ticks);
}

pulse_result_t pulse_generator_pulse_ticks(uint32_t ticks) {
    if (ticks == 0) {
        return PULSE_RESULT_WIDTH_TOO_SHORT;
    }
    // CRITICAL HARDWARE PROTECTION:
    // Interrupts MUST remain disabled during pulse generation to ensure
    // precise timing. Any deviation could cause pulse width >200ns which
    // would DESTROY the discharge circuit hardware. This is a safety requirement.
    noInterrupts();
    
    // Generate pulse with hardware-critical timing precision
    pulse_output_high();
    precise_delay_ticks(ticks);
    pulse_output_low();
    
    // Re-enable interrupts only after pulse is complete
    interrupts();
    return PULSE_RESULT_SUCCESS;
}

pulse_result_t pulse_generator_pulse_minimum(void) {
    return pulse_generator_pulse_ticks(1);
}

uint32_t pulse_generator_get_actual_width_ns(uint32_t requested_ns) {
    uint32_t ticks = timer_lib_ns_to_ticks(requested_ns);
    return timer_lib_ticks_to_ns(ticks);
}

uint32_t pulse_generator_get_resolution_ns(void) {
    return timer_lib_get_max_resolution_ns();
}

uint32_t pulse_generator_get_timer_frequency(void) {
    return timer_lib_get_frequency();
}

const char* pulse_generator_get_platform_info(void) {
    return timer_lib_get_platform_info();
}