/**
 * @file trigger_control.cpp
 * @brief Trigger enable control implementation
 */

#include "trigger_control.h"
#include <Arduino.h>

// =============================================================================
// Private Variables
// =============================================================================

static bool trigger_enabled = false;

// =============================================================================
// Public API Implementation
// =============================================================================

void trigger_control_init(void) {
    pinMode(TRIGGER_ENABLE_PIN, OUTPUT);
    digitalWrite(TRIGGER_ENABLE_PIN, LOW);
    trigger_enabled     = false;
}

void trigger_enable(void) {
    digitalWrite(TRIGGER_ENABLE_PIN, HIGH);
    trigger_enabled = true;
}

void trigger_disable(void) {
    digitalWrite(TRIGGER_ENABLE_PIN, LOW);
    trigger_enabled = false;
}

bool is_trigger_enabled(void) {
    return trigger_enabled;
}