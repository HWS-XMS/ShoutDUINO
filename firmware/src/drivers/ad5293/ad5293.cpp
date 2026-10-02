#include "ad5293.h"
#include <SPI.h>
#include <Arduino.h>

// SPI settings: 500kHz, MSB first, Mode 1 (as per datasheet)
static SPISettings spiSettings(500000, MSBFIRST, SPI_MODE1);

// Cached wiper position (SDO requires external pull-up not present on PCB)
static uint16_t cached_position = 0;

// Command definitions
#define CMD_NOP         0x00
#define CMD_WRITE       0x01
#define CMD_RESET       0x04
#define CMD_WR_CTRL     0x06
#define CMD_SDO_HIGHZ   0x8001
#define CMD_Pos         10
#define DATA_Msk        0x03FF
#define CTRL_WIPER_EN   0x02

static uint16_t combine(uint8_t cmd, uint16_t data) {
    return ((uint16_t)cmd << CMD_Pos) | (data & DATA_Msk);
}

static void ad5293_sdo_highz(void) {
    SPI.beginTransaction(spiSettings);
    digitalWrite(AD5293_CS_PIN, LOW);
    SPI.transfer16(CMD_SDO_HIGHZ);
    digitalWrite(AD5293_CS_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(AD5293_CS_PIN, LOW);
    SPI.transfer16(combine(CMD_NOP, 0));
    digitalWrite(AD5293_CS_PIN, HIGH);
    SPI.endTransaction();
}

void ad5293_init(void) {
    pinMode(AD5293_CS_PIN, OUTPUT);
    digitalWrite(AD5293_CS_PIN, HIGH);
    SPI.begin();
    ad5293_sdo_highz();
    cached_position = 0;
}

bool ad5293_set_position(uint16_t position) {
    if (position > AD5293_MAX_POSITION) {
        return false;
    }

    SPI.beginTransaction(spiSettings);

    // Enable wiper writes
    digitalWrite(AD5293_CS_PIN, LOW);
    SPI.transfer16(combine(CMD_WR_CTRL, CTRL_WIPER_EN));
    digitalWrite(AD5293_CS_PIN, HIGH);
    delayMicroseconds(10);

    // Write position
    digitalWrite(AD5293_CS_PIN, LOW);
    SPI.transfer16(combine(CMD_WRITE, position));
    digitalWrite(AD5293_CS_PIN, HIGH);

    SPI.endTransaction();

    // Put SDO back in high-Z to not interfere with other SPI devices
    ad5293_sdo_highz();

    cached_position = position;
    return true;
}

bool ad5293_get_position(uint16_t *position) {
    if (!position) {
        return false;
    }
    *position = cached_position;
    return true;
}

bool ad5293_set_resistance(uint16_t resistance_ohms) {
    if (resistance_ohms > AD5293_RESISTANCE) {
        return false;
    }
    uint16_t position = (uint32_t)resistance_ohms * AD5293_MAX_POSITION / AD5293_RESISTANCE;
    return ad5293_set_position(position);
}

bool ad5293_get_resistance(uint16_t *resistance_ohms) {
    if (!resistance_ohms) {
        return false;
    }
    *resistance_ohms = (uint32_t)cached_position * AD5293_RESISTANCE / AD5293_MAX_POSITION;
    return true;
}

bool ad5293_reset(void) {
    SPI.beginTransaction(spiSettings);
    digitalWrite(AD5293_CS_PIN, LOW);
    SPI.transfer16(combine(CMD_RESET, 0));
    digitalWrite(AD5293_CS_PIN, HIGH);
    SPI.endTransaction();

    // Put SDO back in high-Z to not interfere with other SPI devices
    ad5293_sdo_highz();

    cached_position = 512;  // Reset goes to midscale
    return true;
}
