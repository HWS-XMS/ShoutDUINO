/**
 * @file mcp3201.h
 * @brief MCP3201 12-bit ADC driver for high voltage measurement
 * @details Single-ended 12-bit ADC with SPI interface
 */

#ifndef MCP3201_H
#define MCP3201_H

#include <stdint.h>
#include <stdbool.h>
#include "../../core/config/pin_config.h"

// =============================================================================
// MCP3201 Configuration
// =============================================================================

#define MCP3201_RESOLUTION      12
#define MCP3201_MAX_VALUE       4095
#define MCP3201_VREF_V          3.279f
#define MCP3201_LSB_SIZE_V      (MCP3201_VREF_V / 4096.0f)

// High voltage measurement circuit (5M / 25k divider)
#define HV_R_HIGH_OHM           5000000UL
#define HV_R_LOW_OHM            25000UL
#define HV_DIVIDER_RATIO        ((float)(HV_R_HIGH_OHM + HV_R_LOW_OHM) / HV_R_LOW_OHM)

// =============================================================================
// MCP3201 API
// =============================================================================

/**
 * @brief Initialize MCP3201 ADC driver
 * @return true on success, false on failure
 */
void mcp3201_init(void);

/**
 * @brief Read raw ADC value
 * @param raw_value Pointer to store 12-bit ADC result (0-4095)
 * @return true on success, false on failure
 */
bool mcp3201_read_raw(uint16_t *raw_value);

/**
 * @brief Read raw ADC bytes for debugging
 * @param hi_byte Pointer to store high byte from SPI
 * @param lo_byte Pointer to store low byte from SPI
 * @param raw_value Pointer to store extracted 12-bit result
 * @return true on success, false on failure
 */
bool mcp3201_read_raw_debug(uint8_t *hi_byte, uint8_t *lo_byte, uint16_t *raw_value);

/**
 * @brief Software SPI read for timing verification
 * @param raw_value Pointer to store 12-bit ADC result
 * @return true on success
 */
bool mcp3201_read_raw_sw(uint16_t *raw_value);

/**
 * @brief Read voltage at ADC input
 * @param voltage_v Pointer to store voltage in volts
 * @return true on success, false on failure
 */
bool mcp3201_read_voltage(float *voltage_v);

/**
 * @brief Read high voltage after voltage divider scaling
 * @param hv_voltage_v Pointer to store high voltage in volts
 * @return true on success, false on failure
 */
bool mcp3201_read_hv_voltage(float *hv_voltage_v);

#endif // MCP3201_H