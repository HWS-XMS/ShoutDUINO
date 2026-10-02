#include "led_indicators.h"
#include <Arduino.h>

void led_indicators_init(void) {
    pinMode(LED_HV_MEASURED_PIN, OUTPUT);
    pinMode(LED_HT_MEASURED_PIN, OUTPUT);
    digitalWrite(LED_HV_MEASURED_PIN, LOW);
    digitalWrite(LED_HT_MEASURED_PIN, LOW);
}

void led_hv_measured_on(void) {
    digitalWrite(LED_HV_MEASURED_PIN, HIGH);
}

void led_hv_measured_off(void) {
    digitalWrite(LED_HV_MEASURED_PIN, LOW);
}

void led_ht_measured_on(void) {
    digitalWrite(LED_HT_MEASURED_PIN, HIGH);
}

void led_ht_measured_off(void) {
    digitalWrite(LED_HT_MEASURED_PIN, LOW);
}