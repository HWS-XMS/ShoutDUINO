/**
 * @file uart_protocol.cpp
 * @brief UART communication protocol implementation with interrupt handling
 */

#include "uart_protocol.h"
#include <Arduino.h>

// =============================================================================
// Private Variables
// =============================================================================

static bool initialized = false;
static uart_frame_callback_t frame_callback = nullptr;

// Reception state machine
static uart_rx_state_t rx_state = UART_STATE_IDLE;
static uart_frame_t rx_frame;
static uint8_t rx_data_index = 0;
static uint16_t calculated_crc = 0;

// Statistics
static uint32_t frame_count = 0;
static uint32_t crc_error_count = 0;

// Circular buffer for incoming bytes
static uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t rx_buffer_head = 0;
static volatile uint16_t rx_buffer_tail = 0;

// =============================================================================
// CRC16 Calculation (CCITT-16)
// =============================================================================

static uint16_t crc16_update(uint16_t crc, uint8_t data)
{
    crc ^= (uint16_t)data << 8;
    for (uint8_t i = 0; i < 8; i++) {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ 0x1021;
        } else {
            crc = crc << 1;
        }
    }
    return crc & 0xFFFF;
}

static uint16_t calculate_frame_crc(const uart_frame_t* frame)
{
    uint16_t crc = 0;
    crc = crc16_update(crc, frame->start_byte);
    crc = crc16_update(crc, frame->command);
    crc = crc16_update(crc, frame->length);
    
    for (uint8_t i = 0; i < frame->length; i++) {
        crc = crc16_update(crc, frame->data[i]);
    }
    
    return crc;
}

// =============================================================================
// Buffer Management
// =============================================================================

static bool rx_buffer_available(void)
{
    return (rx_buffer_head != rx_buffer_tail);
}

static uint8_t rx_buffer_read(void)
{
    if (rx_buffer_head == rx_buffer_tail) {
        return 0;  // Buffer empty
    }
    
    uint8_t data = rx_buffer[rx_buffer_tail];
    rx_buffer_tail = (rx_buffer_tail + 1) % UART_RX_BUFFER_SIZE;
    return data;
}

static void rx_buffer_write(uint8_t data)
{
    uint16_t next_head = (rx_buffer_head + 1) % UART_RX_BUFFER_SIZE;
    
    if (next_head != rx_buffer_tail) {  // Buffer not full
        rx_buffer[rx_buffer_head] = data;
        rx_buffer_head = next_head;
    }
    // If buffer full, data is dropped (could add overflow counter)
}

// =============================================================================
// Platform-Specific Interrupt Handlers
// =============================================================================

#ifdef ARDUINO_AVR_UNO
// Use Arduino's serialEvent() instead of direct ISR to avoid conflicts
void serialEvent()
{
    while (Serial.available()) {
        uint8_t data = Serial.read();
        rx_buffer_write(data);
    }
}

static void enable_uart_rx_interrupt(void)
{
    // serialEvent() is called automatically by Arduino framework
}

static void disable_uart_rx_interrupt(void)
{
    // serialEvent() is managed by Arduino framework
}

#elif defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
// Arduino R4 UART interrupt (using Arduino core functions)
void serialEvent()
{
    while (Serial.available()) {
        uint8_t data = Serial.read();
        rx_buffer_write(data);
    }
}

static void enable_uart_rx_interrupt(void)
{
    // R4 uses serialEvent() callback - enabled automatically when defined
}

static void disable_uart_rx_interrupt(void)
{
    // R4 interrupt handling managed by Arduino core
}

#else
// Generic fallback - polling mode
static void enable_uart_rx_interrupt(void) { }
static void disable_uart_rx_interrupt(void) { }
#endif

// =============================================================================
// Frame Processing State Machine
// =============================================================================

static void process_received_byte(uint8_t data)
{
    switch (rx_state) {
        case UART_STATE_IDLE:
            if (data == UART_FRAME_START_BYTE) {
                rx_frame.start_byte = data;
                calculated_crc = crc16_update(0, data);
                rx_state = UART_STATE_COMMAND;
            }
            break;
            
        case UART_STATE_COMMAND:
            rx_frame.command = data;
            calculated_crc = crc16_update(calculated_crc, data);
            rx_state = UART_STATE_LENGTH;
            break;
            
        case UART_STATE_LENGTH:
            rx_frame.length = data;
            calculated_crc = crc16_update(calculated_crc, data);
            
            if (rx_frame.length <= UART_MAX_DATA_LENGTH) {
                rx_data_index = 0;
                if (rx_frame.length > 0) {
                    rx_state = UART_STATE_DATA;
                } else {
                    rx_state = UART_STATE_CRC_HIGH;
                }
            } else {
                // Invalid length - reset state machine
                rx_state = UART_STATE_IDLE;
            }
            break;
            
        case UART_STATE_DATA:
            rx_frame.data[rx_data_index] = data;
            calculated_crc = crc16_update(calculated_crc, data);
            rx_data_index++;
            
            if (rx_data_index >= rx_frame.length) {
                rx_state = UART_STATE_CRC_HIGH;
            }
            break;
            
        case UART_STATE_CRC_HIGH:
            rx_frame.crc16 = (uint16_t)data << 8;
            rx_state = UART_STATE_CRC_LOW;
            break;
            
        case UART_STATE_CRC_LOW:
            rx_frame.crc16 |= data;
            rx_state = UART_STATE_END;
            break;
            
        case UART_STATE_END:
            rx_frame.end_byte = data;
            
            if (data == UART_FRAME_END_BYTE) {
                // Verify CRC
                if (rx_frame.crc16 == calculated_crc) {
                    rx_state = UART_STATE_COMPLETE;
                    frame_count++;
                    
                    // Call frame callback if registered
                    if (frame_callback) {
                        frame_callback(&rx_frame);
                    }
                } else {
                    // CRC error
                    crc_error_count++;
                }
            }
            
            // Reset state machine for next frame
            rx_state = UART_STATE_IDLE;
            break;
            
        case UART_STATE_COMPLETE:
            // This state is handled by the callback
            rx_state = UART_STATE_IDLE;
            break;
    }
}

// =============================================================================
// Public API Implementation
// =============================================================================

bool uart_protocol_init(uart_frame_callback_t callback)
{
    if (initialized) {
        return true;
    }
    
    // Initialize serial communication
    Serial.begin(UART_BAUDRATE);
    
    // Store callback function
    frame_callback = callback;
    
    // Initialize state machine
    rx_state = UART_STATE_IDLE;
    rx_data_index = 0;
    calculated_crc = 0;
    
    // Initialize statistics
    frame_count = 0;
    crc_error_count = 0;
    
    // Initialize buffer
    rx_buffer_head = 0;
    rx_buffer_tail = 0;
    
    // Enable UART receive interrupt
    enable_uart_rx_interrupt();
    
    initialized = true;
    return true;
}

bool uart_send_frame(uart_command_t command, const uint8_t* data, uint8_t length)
{
    if (!initialized || length > UART_MAX_DATA_LENGTH) {
        return false;
    }
    
    uart_frame_t frame;
    frame.start_byte = UART_FRAME_START_BYTE;
    frame.command = (uint8_t)command;
    frame.length = length;
    
    // Copy data payload
    if (data && length > 0) {
        memcpy(frame.data, data, length);
    }
    
    // Calculate CRC
    frame.crc16 = calculate_frame_crc(&frame);
    frame.end_byte = UART_FRAME_END_BYTE;
    
    // Send frame
    Serial.write(frame.start_byte);
    Serial.write(frame.command);
    Serial.write(frame.length);
    
    if (length > 0) {
        Serial.write(frame.data, length);
        Serial.flush();  // Force transmission of data
    }
    
    Serial.write((uint8_t)(frame.crc16 >> 8));   // CRC high byte
    Serial.write((uint8_t)(frame.crc16 & 0xFF)); // CRC low byte
    Serial.write(frame.end_byte);
    Serial.flush();  // Ensure complete transmission
    
    return true;
}

bool uart_send_response(uart_response_t response, const uint8_t* data, uint8_t length)
{
    return uart_send_frame((uart_command_t)response, data, length);
}

void uart_protocol_process(void)
{
    if (!initialized) {
        return;
    }
    
    // Process all available bytes from interrupt buffer
    while (rx_buffer_available()) {
        uint8_t data = rx_buffer_read();
        process_received_byte(data);
    }
    
    // Handle serialEvent for R4 platforms
    #if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
    if (Serial.available()) {
        serialEvent();
    }
    #endif
}

bool uart_is_busy(void)
{
    // Check if serial transmission is complete
    return !Serial.availableForWrite();
}

uint32_t uart_get_frame_count(void)
{
    return frame_count;
}

uint32_t uart_get_crc_error_count(void)
{
    return crc_error_count;
}