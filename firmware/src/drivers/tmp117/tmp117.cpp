#include "tmp117.h"
#include <Wire.h>
#include <Arduino.h>

static bool tmp117_write_register(uint8_t i2c_address, uint8_t reg_addr, uint16_t value) {
    Wire.beginTransmission(i2c_address);
    Wire.write(reg_addr);
    Wire.write((uint8_t)(value >> 8));    // MSB first
    Wire.write((uint8_t)(value & 0xFF));  // LSB second
    return (Wire.endTransmission() == 0);
}

static bool tmp117_read_register(uint8_t i2c_address, uint8_t reg_addr, uint16_t *value) {
    if (!value) {
        return false;
    }
    Wire.beginTransmission(i2c_address);
    Wire.write(reg_addr);
    if (Wire.endTransmission() != 0){
        return false;
    }
    Wire.requestFrom(i2c_address, (uint8_t)2);
    if (Wire.available() != 2){
        return false;
    }
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    *value = ((uint16_t)msb << 8) | lsb;
    return true;
}

bool tmp117_init(uint8_t i2c_address) {
    Wire.begin();
    uint16_t device_id;
    if (!tmp117_read_register(i2c_address, TMP117_REG_DEVICE_ID, &device_id)) {
        return false;
    }
    if (device_id != TMP117_DEVICE_ID_VALUE) {
        return false;
    }
    if (!tmp117_write_register(i2c_address, TMP117_REG_CONFIGURATION, TMP117_CFG_DEFAULT)) {
        return false;
    }
    delay(1100);
    return true;
}

bool tmp117_read_temperature(uint8_t i2c_address, float *temperature_c) {
    if (!temperature_c) {
        return false;
    }
    uint16_t temp_reg;
    if (!tmp117_read_register(i2c_address, TMP117_REG_TEMP_RESULT, &temp_reg)) {
        return false;
    }
    *temperature_c = (float)((int16_t)temp_reg) * TMP117_RESOLUTION;
    return true;
}

bool tmp117_set_alert_high(uint8_t i2c_address, float temp_high_c) {
    int16_t temp_raw = (int16_t)(temp_high_c / TMP117_RESOLUTION);
    return tmp117_write_register(i2c_address, TMP117_REG_THIGH_LIMIT, (uint16_t)temp_raw);
}

bool tmp117_set_alert_polarity(uint8_t i2c_address, bool active_high) {
    uint16_t config;
    if (!tmp117_read_register(i2c_address, TMP117_REG_CONFIGURATION, &config)) {
        return false;
    }
    if (active_high) {
        config |= TMP117_CFG_POL;
    } else {
        config &= ~TMP117_CFG_POL;
    }
    return tmp117_write_register(i2c_address, TMP117_REG_CONFIGURATION, config);
}

bool tmp117_is_alert_active(uint8_t i2c_address) {
    uint16_t config;
    if (!tmp117_read_register(i2c_address, TMP117_REG_CONFIGURATION, &config)) {
        return false;
    }
    return (config & TMP117_CFG_HIGH_ALERT) != 0;
}