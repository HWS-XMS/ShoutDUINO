#include "temperature_controller.h"
#include "../../drivers/tmp117/tmp117.h"
#include "../config/pin_config.h"
#include <Arduino.h>

// Sensor I2C addresses
static const uint8_t sensor_addresses[TEMP_SENSOR_COUNT] = {
    TMP117_U1_ADDRESS,   // Primary NMOS
    TMP117_U2_ADDRESS,   // Gate driver
    TMP117_U3_ADDRESS,   // Transformer
    TMP117_U14_ADDRESS   // Discharge NMOS
};

// Default thresholds
static const float default_thresholds[TEMP_SENSOR_COUNT] = {
    TEMP_THRESHOLD_PRIMARY_NMOS,
    TEMP_THRESHOLD_GATE_DRIVER,
    TEMP_THRESHOLD_TRANSFORMER,
    TEMP_THRESHOLD_DISCHARGE_NMOS
};

temp_error_code_t temperature_controller_init(void) {
    // Initialize Primary NMOS sensor
    if (!tmp117_init(TMP117_U1_ADDRESS)) {
        return TEMP_ERR_PRIMARY_NMOS_INIT;
    }
    if (!tmp117_set_alert_high(TMP117_U1_ADDRESS, TEMP_THRESHOLD_PRIMARY_NMOS)) {
        return TEMP_ERR_PRIMARY_NMOS_SET_ALERT;
    }
    if (!tmp117_set_alert_polarity(TMP117_U1_ADDRESS, true)) {
        return TEMP_ERR_PRIMARY_NMOS_SET_POLARITY;
    }

    // Initialize Gate driver sensor
    if (!tmp117_init(TMP117_U2_ADDRESS)) {
        return TEMP_ERR_GATE_DRIVER_INIT;
    }
    if (!tmp117_set_alert_high(TMP117_U2_ADDRESS, TEMP_THRESHOLD_GATE_DRIVER)) {
        return TEMP_ERR_GATE_DRIVER_SET_ALERT;
    }
    if (!tmp117_set_alert_polarity(TMP117_U2_ADDRESS, true)) {
        return TEMP_ERR_GATE_DRIVER_SET_POLARITY;
    }

    // Initialize Transformer sensor
    if (!tmp117_init(TMP117_U3_ADDRESS)) {
        return TEMP_ERR_TRANSFORMER_INIT;
    }
    if (!tmp117_set_alert_high(TMP117_U3_ADDRESS, TEMP_THRESHOLD_TRANSFORMER)) {
        return TEMP_ERR_TRANSFORMER_SET_ALERT;
    }
    if (!tmp117_set_alert_polarity(TMP117_U3_ADDRESS, true)) {
        return TEMP_ERR_TRANSFORMER_SET_POLARITY;
    }

    // Initialize Discharge NMOS sensor
    if (!tmp117_init(TMP117_U14_ADDRESS)) {
        return TEMP_ERR_DISCHARGE_NMOS_INIT;
    }
    if (!tmp117_set_alert_high(TMP117_U14_ADDRESS, TEMP_THRESHOLD_DISCHARGE_NMOS)) {
        return TEMP_ERR_DISCHARGE_NMOS_SET_ALERT;
    }
    if (!tmp117_set_alert_polarity(TMP117_U14_ADDRESS, true)) {
        return TEMP_ERR_DISCHARGE_NMOS_SET_POLARITY;
    }

    pinMode(TEMP_ALERT_PIN, INPUT_PULLDOWN);
    return TEMP_ERR_OK;
}

bool temperature_read_all(float temperatures[TEMP_SENSOR_COUNT]) {
    if (!temperatures) return false;
    for (int i = 0; i < TEMP_SENSOR_COUNT; i++) {
        if (!tmp117_read_temperature(sensor_addresses[i], &temperatures[i])) {
            return false;
        }
    }
    return true;
}

bool temperature_check_alert(void) {
    return digitalRead(TEMP_ALERT_PIN) == HIGH;
}

bool temperature_set_thresholds(float thresholds[TEMP_SENSOR_COUNT]) {
    if (!thresholds) return false;
    for (int i = 0; i < TEMP_SENSOR_COUNT; i++) {
        if (!tmp117_set_alert_high(sensor_addresses[i], thresholds[i])) {
            return false;
        }
    }
    return true;
}