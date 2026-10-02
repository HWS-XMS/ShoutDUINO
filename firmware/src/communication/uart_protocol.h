/**
 * @file uart_protocol.h
 * @brief UART communication protocol with interrupt-driven frame handling
 * @details Frame-based protocol for reliable communication with external control systems
 */

#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Protocol Constants
// =============================================================================

#define UART_FRAME_START_BYTE   0xAA    // Frame start marker
#define UART_FRAME_END_BYTE     0x55    // Frame end marker
#define UART_MAX_DATA_LENGTH    64      // Maximum payload length
#define UART_BAUDRATE           115200  // Communication speed
#define UART_RX_BUFFER_SIZE     256     // Receive buffer size

// =============================================================================
// Frame Structure
// =============================================================================

typedef struct {
    uint8_t start_byte;                     // Frame start (0xAA)
    uint8_t command;                        // Command type
    uint8_t length;                         // Data payload length
    uint8_t data[UART_MAX_DATA_LENGTH];     // Command payload
    uint16_t crc16;                         // CRC16 checksum
    uint8_t end_byte;                       // Frame end (0x55)
} __attribute__((packed)) uart_frame_t;

#define UART_FRAME_OVERHEAD     6       // start + cmd + len + crc16 + end

// =============================================================================
// Command Types
// =============================================================================

typedef enum {
    // System Commands
    UART_CMD_INIT           = 0x01,     // Initialize system with default values
    UART_CMD_GET_STATUS     = 0x02,     // Get system status
    UART_CMD_GET_DEVICE_INFO = 0x03,    // Get device info (platform, capabilities)
    
    // Charging Commands
    UART_CMD_CHARGE_START   = 0x10,     // Start charging
    UART_CMD_CHARGE_STOP    = 0x11,     // Stop charging
    UART_CMD_SET_VOLTAGE    = 0x12,     // Set target voltage (requires calibration)
    UART_CMD_GET_VOLTAGE    = 0x13,     // Get target voltage setting
    UART_CMD_GET_VOLTAGE_MEASURED = 0x14, // Get measured high voltage
    UART_CMD_SET_POT_POSITION = 0x15,   // Set potentiometer position (0-1023)
    UART_CMD_GET_POT_POSITION = 0x16,   // Get potentiometer position
    UART_CMD_GET_ADC_DEBUG    = 0x17,   // Get raw ADC bytes for debugging
    UART_CMD_GET_ADC_SW       = 0x18,   // Get ADC value using software SPI

    // Gate Driver Commands
    UART_CMD_SET_GATE_DRIVER_EN = 0x20, // Set gate driver enable
    UART_CMD_GET_GATE_DRIVER_EN = 0x21, // Get gate driver enable state
    
    // Trigger Commands
    UART_CMD_SET_TRIGGER_EN = 0x22,     // Set trigger enable
    UART_CMD_GET_TRIGGER_EN = 0x23,     // Get trigger enable state
    UART_CMD_SOFT_TRIGGER   = 0x24,     // Fire soft trigger pulse
    UART_CMD_SET_SOFT_TRIGGER_WIDTH = 0x25, // Set soft trigger pulse width
    UART_CMD_GET_SOFT_TRIGGER_WIDTH = 0x26, // Get soft trigger pulse width
    
    // Temperature Commands
    UART_CMD_READ_TEMPERATURES = 0x30,  // Read all temperature sensors
    UART_CMD_SET_TEMP_LIMITS = 0x31     // Set critical temperature limits
} uart_command_t;

// =============================================================================
// Response Types
// =============================================================================

typedef enum {
    UART_RESP_OK            = 0x00,     // Command successful
    UART_RESP_ERROR         = 0x01,     // General error
    UART_RESP_INVALID_CMD   = 0x02,     // Unknown command
    UART_RESP_INVALID_CRC   = 0x03,     // CRC mismatch
    UART_RESP_BUSY          = 0x04,     // System busy
    UART_RESP_FAULT         = 0x05      // Hardware fault detected
} uart_response_t;

// =============================================================================
// Frame Processing State
// =============================================================================

typedef enum {
    UART_STATE_IDLE = 0,        // Waiting for start byte
    UART_STATE_COMMAND,         // Receiving command byte
    UART_STATE_LENGTH,          // Receiving length byte
    UART_STATE_DATA,            // Receiving data payload
    UART_STATE_CRC_HIGH,        // Receiving CRC high byte
    UART_STATE_CRC_LOW,         // Receiving CRC low byte
    UART_STATE_END,             // Receiving end byte
    UART_STATE_COMPLETE         // Frame complete
} uart_rx_state_t;

// =============================================================================
// Callback Function Type
// =============================================================================

typedef void (*uart_frame_callback_t)(const uart_frame_t* frame);

// =============================================================================
// UART Protocol API
// =============================================================================

/**
 * @brief Initialize UART communication with interrupt handling
 * @param callback Function to call when complete frame received
 * @return true on success, false on failure
 */
bool uart_protocol_init(uart_frame_callback_t callback);

/**
 * @brief Send a frame over UART
 * @param command Command type
 * @param data Payload data (can be nullptr if length is 0)
 * @param length Payload length
 * @return true on success, false on failure
 */
bool uart_send_frame(uart_command_t command, const uint8_t* data, uint8_t length);

/**
 * @brief Send a response frame
 * @param response Response code
 * @param data Optional response data
 * @param length Response data length
 * @return true on success, false on failure
 */
bool uart_send_response(uart_response_t response, const uint8_t* data, uint8_t length);

/**
 * @brief Process received bytes (call from main loop)
 * @details Handles frame processing outside of interrupt context
 */
void uart_protocol_process(void);

/**
 * @brief Check if UART is busy transmitting
 * @return true if transmission in progress
 */
bool uart_is_busy(void);

/**
 * @brief Get number of frames received
 * @return Frame count since initialization
 */
uint32_t uart_get_frame_count(void);

/**
 * @brief Get number of CRC errors detected
 * @return CRC error count
 */
uint32_t uart_get_crc_error_count(void);

#endif // UART_PROTOCOL_H