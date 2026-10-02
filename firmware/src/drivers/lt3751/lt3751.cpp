#include "lt3751.h"
#include <Arduino.h>

static bool charging_enabled = false;

void lt3751_init(void) {
    pinMode(LT3751_CHARGE_PIN, OUTPUT);
    pinMode(LT3751_nDONE_PIN, INPUT);
    pinMode(LT3751_nFAULT_PIN, INPUT);
    digitalWrite(LT3751_CHARGE_PIN, LOW);
    charging_enabled = false;
}

bool lt3751_start_charging(void) {
    if (!charging_enabled) {
        digitalWrite(LT3751_CHARGE_PIN, HIGH);
        charging_enabled = true;
    }
    return true;
}

void lt3751_stop_charging(void) {
    digitalWrite(LT3751_CHARGE_PIN, LOW);
    charging_enabled = false;
}

bool lt3751_is_charging(void) {
    return charging_enabled && (digitalRead(LT3751_CHARGE_PIN) == HIGH);
}

bool lt3751_has_fault(void) {
    // nFAULT is active low - fault when pin reads LOW
    return (digitalRead(LT3751_nFAULT_PIN) == LOW);
}

bool lt3751_is_done(void) {
    // nDONE is active low - done when pin reads LOW
    // Note: In feedback auto-regulated mode, this should typically not occur
    return (digitalRead(LT3751_nDONE_PIN) == LOW);
}

bool lt3751_clear_fault(void) {
    if (!lt3751_has_fault()) {
        return true;  // No fault to clear
    }
    lt3751_stop_charging();
    delay(100);
    if (!lt3751_has_fault()) {
        return true; // Fault cleared
    }    
    // Fault persists
    return false;
}