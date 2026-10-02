#include "mcp3201.h"
#include <SPI.h>
#include <Arduino.h>

static void mcp3201_cs_select(void) {
    digitalWrite(MCP3201_CS_PIN, LOW);  // Pull CS low
    delayMicroseconds(1);  // CS setup time
}

static void mcp3201_cs_deselect(void) {
    digitalWrite(MCP3201_CS_PIN, HIGH);   // Pull CS high
    delayMicroseconds(1);  // CS hold time
}

void mcp3201_init(void) {
    pinMode(MCP3201_CS_PIN, OUTPUT);
    digitalWrite(MCP3201_CS_PIN, HIGH);
    SPI.begin();
}

bool mcp3201_read_raw(uint16_t *raw_value)
{
    if (!raw_value) {
        return false;
    }

    // Configure SPI for MCP3201 (Mode 0, 500kHz - max 800kHz at 3.3V VREF)
    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));

    digitalWrite(MCP3201_CS_PIN, LOW);
    delayMicroseconds(1);
    uint8_t hi = SPI.transfer(0x00);
    uint8_t lo = SPI.transfer(0x00);
    digitalWrite(MCP3201_CS_PIN, HIGH);

    SPI.endTransaction();

    // MCP3201 format after 16 clocks (Mode 0, data sampled on rising edge):
    // hi = [?][?][Null][B11][B10][B9][B8][B7]
    // lo = [B6][B5][B4][B3][B2][B1][B0][B1]
    uint16_t reading = ((hi & 0x1F) << 7) | (lo >> 1);

    *raw_value = reading;
    return true;
}

// Store last raw bytes for debugging
static uint8_t last_hi = 0, last_lo = 0;
static uint16_t last_raw = 0;

bool mcp3201_read_raw_debug(uint8_t *hi_byte, uint8_t *lo_byte, uint16_t *raw_value)
{
    if (!hi_byte || !lo_byte || !raw_value) {
        return false;
    }

    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));

    digitalWrite(MCP3201_CS_PIN, LOW);
    delayMicroseconds(1);
    uint8_t hi = SPI.transfer(0x00);
    uint8_t lo = SPI.transfer(0x00);
    digitalWrite(MCP3201_CS_PIN, HIGH);

    SPI.endTransaction();

    uint16_t reading = ((hi & 0x1F) << 7) | (lo >> 1);

    *hi_byte = hi;
    *lo_byte = lo;
    *raw_value = reading;
    return true;
}

// Software SPI read for timing verification (bit-bang at ~100kHz)
bool mcp3201_read_raw_sw(uint16_t *raw_value)
{
    if (!raw_value) {
        return false;
    }

    // Use software SPI for precise timing control
    pinMode(SPI_SCK_PIN, OUTPUT);
    pinMode(SPI_MISO_PIN, INPUT);

    // Ensure CS starts high (device in standby between conversions)
    digitalWrite(MCP3201_CS_PIN, HIGH);
    delayMicroseconds(10);

    digitalWrite(SPI_SCK_PIN, LOW);  // Clock idles low (Mode 0)
    delayMicroseconds(1);

    digitalWrite(MCP3201_CS_PIN, LOW);  // Start conversion
    delayMicroseconds(1);  // CS setup time (100ns min)

    uint16_t data = 0;

    // Read 16 bits, MSB first
    for (int i = 0; i < 16; i++) {
        digitalWrite(SPI_SCK_PIN, HIGH);  // Rising edge - sample
        delayMicroseconds(5);

        data <<= 1;
        if (digitalRead(SPI_MISO_PIN)) {
            data |= 1;
        }

        digitalWrite(SPI_SCK_PIN, LOW);   // Falling edge - data changes
        delayMicroseconds(5);
    }

    digitalWrite(MCP3201_CS_PIN, HIGH);  // End conversion, return to standby
    delayMicroseconds(1);

    // Restore hardware SPI
    SPI.begin();

    // Extract 12-bit value: bits [12:1] contain B11-B0
    // data = [?][?][Null][B11][B10][B9][B8][B7][B6][B5][B4][B3][B2][B1][B0][B0]
    *raw_value = (data >> 1) & 0x0FFF;

    return true;
}

bool mcp3201_read_voltage(float *voltage_v)
{
    if (!voltage_v) {
        return false;
    }
    
    uint16_t raw_value;
    if (!mcp3201_read_raw(&raw_value)) {
        return false;
    }
    
    // Convert to voltage using VREF
    *voltage_v = (float)raw_value * MCP3201_LSB_SIZE_V;
    return true;
}

bool mcp3201_read_hv_voltage(float *hv_voltage_v)
{
    if (!hv_voltage_v) {
        return false;
    }
    
    float adc_voltage;
    if (!mcp3201_read_voltage(&adc_voltage)) {
        return false;
    }
    
    // Scale by voltage divider ratio to get actual high voltage
    *hv_voltage_v = adc_voltage * HV_DIVIDER_RATIO;
    return true;
}