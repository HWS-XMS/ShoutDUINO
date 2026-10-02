#include "gate_driver.h"
#include <Arduino.h>

static bool driver_enabled = false;

void gate_driver_init(void) {
    pinMode(GATE_DRIVER_EN_PIN, OUTPUT);
    digitalWrite(GATE_DRIVER_EN_PIN, LOW);
    driver_enabled = false;
}

void gate_driver_enable(void) {
    digitalWrite(GATE_DRIVER_EN_PIN, HIGH);
    driver_enabled = true;
}

void gate_driver_disable(void) {
    digitalWrite(GATE_DRIVER_EN_PIN, LOW);
    driver_enabled = false;
}

bool is_gate_driver_enabled(void) {
    return driver_enabled;
}