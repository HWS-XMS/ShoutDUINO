#include "src/core/temperature/temperature_controller.h"
#include "src/drivers/mcp3201/mcp3201.h"
#include "src/drivers/ad5293/ad5293.h"
#include "src/drivers/lt3751/lt3751.h"
#include "src/drivers/gate_driver/gate_driver.h"
#include "src/drivers/trigger/trigger_control.h"
#include "src/core/pulse_generator/pulse_generator.h"
#include "src/drivers/leds/led_indicators.h"
#include "src/communication/uart_protocol.h"
#include "src/drivers/timer/timer_lib.h"

#define SAFE_VOLTAGE                10.0f
#define VOLTAGE_DEFAULT_TARGET      100.0f
#define MEASUREMENT_INTERVAL_MS     100

static float target_voltage                 = VOLTAGE_DEFAULT_TARGET;
static uint32_t soft_trigger_width_ns       = 100;
static unsigned long last_measurement_time  = 0;
static unsigned long last_led_update_time   = 0;
static float current_temperatures[4]        = {0.0f, 0.0f, 0.0f, 0.0f};
static float current_voltage                = 0.0f;
static bool temperature_alert               = false;
static bool high_voltage                    = false;
static temp_error_code_t last_temp_error    = TEMP_ERR_OK;

bool system_init(void) {
    last_temp_error = temperature_controller_init();
    if (last_temp_error != TEMP_ERR_OK) {
        return false;
    }

    ad5293_init();
    timer_lib_init();
    pulse_generator_init();
    led_indicators_init();
    mcp3201_init();
    lt3751_init();
    trigger_control_init();
    gate_driver_init();

    return true;
}

void update_measurements(void) {
    temperature_read_all(current_temperatures);
    if (mcp3201_read_hv_voltage(&current_voltage)) {
        high_voltage = current_voltage > SAFE_VOLTAGE;
    }
    temperature_alert = temperature_check_alert();
}

void update_led_indicators(void) {
    if (high_voltage) {
        led_hv_measured_on();
    } else {
        led_hv_measured_off();
    }
    if (temperature_alert) {
        led_ht_measured_on();
    } else {
        led_ht_measured_off();
    }
}

void handle_uart_frame(const uart_frame_t* frame) {
    switch (frame->command) {
        
        case UART_CMD_INIT: {
            if (system_init()) {
                uart_send_response(UART_RESP_OK, nullptr, 0);
            } else {
                // Send temperature error code as single byte
                uint8_t error_code = (uint8_t)last_temp_error;
                uart_send_response(UART_RESP_ERROR, &error_code, 1);
            }
            break;
        }
            
        case UART_CMD_GET_STATUS: {
            // Status format: [flags(1)] [pot(2)] [voltage(4)] [pulse_width(4)] [temps(16)] = 27 bytes
            uint8_t status_data[27];

            // Byte 0: flags
            uint8_t flags = 0;
            if (lt3751_is_charging())       flags |= 0x01;
            if (lt3751_has_fault())         flags |= 0x02;
            if (lt3751_is_done())           flags |= 0x04;
            if (is_gate_driver_enabled())   flags |= 0x08;
            if (is_trigger_enabled())       flags |= 0x10;
            if (temperature_alert)          flags |= 0x20;
            status_data[0] = flags;

            // Bytes 1-2: pot position
            uint16_t pot_pos;
            ad5293_get_position(&pot_pos);
            status_data[1] = pot_pos & 0xFF;
            status_data[2] = (pot_pos >> 8) & 0xFF;

            // Bytes 3-6: measured voltage
            memcpy(&status_data[3], &current_voltage, 4);

            // Bytes 7-10: pulse width
            memcpy(&status_data[7], &soft_trigger_width_ns, 4);

            // Bytes 11-26: temperatures (4 floats)
            memcpy(&status_data[11], current_temperatures, 16);

            uart_send_response(UART_RESP_OK, status_data, sizeof(status_data));
            break;
        }
            
        case UART_CMD_GET_DEVICE_INFO: {
            const char* platform_info = timer_lib_get_platform_info();
            uart_send_response(UART_RESP_OK, (const uint8_t*)platform_info, strlen(platform_info));
            break;
        }
            
        case UART_CMD_CHARGE_START: {
            if (lt3751_start_charging()) {
                uart_send_response(UART_RESP_OK, nullptr, 0);
            } else {
                uart_send_response(UART_RESP_FAULT, nullptr, 0);
            }
            break;
        }
            
        case UART_CMD_CHARGE_STOP: {
            lt3751_stop_charging();
            uart_send_response(UART_RESP_OK, nullptr, 0);
            break;
        }
            
        case UART_CMD_SET_GATE_DRIVER_EN: {
            if (frame->length >= 1) {
                if (frame->data[0]) {
                    gate_driver_enable();
                } else {
                    gate_driver_disable();
                }
                uart_send_response(UART_RESP_OK, nullptr, 0);
            } else {
                uart_send_response(UART_RESP_ERROR, nullptr, 0);
            }
            break;
        }
            
        case UART_CMD_GET_GATE_DRIVER_EN: {
            uint8_t state = is_gate_driver_enabled() ? 1 : 0;
            uart_send_response(UART_RESP_OK, &state, 1);
            break;
        }
            
        case UART_CMD_SET_TRIGGER_EN: {
            if (frame->length >= 1) {
                if (frame->data[0]) {
                    trigger_enable();
                } else {
                    trigger_disable();
                }
                uart_send_response(UART_RESP_OK, nullptr, 0);
            } else {
                uart_send_response(UART_RESP_ERROR, nullptr, 0);
            }
            break;
        }
            
        case UART_CMD_GET_TRIGGER_EN: {
            uint8_t state = is_trigger_enabled() ? 1 : 0;
            uart_send_response(UART_RESP_OK, &state, 1);
            break;
        }
            
        case UART_CMD_SOFT_TRIGGER: {
            pulse_result_t result = pulse_generator_pulse(soft_trigger_width_ns);
            if (result == PULSE_RESULT_SUCCESS) {
                uart_send_response(UART_RESP_OK, nullptr, 0);
            } else {
                uart_send_response(UART_RESP_ERROR, nullptr, 0);
            }
            break;
        }
            
        case UART_CMD_SET_SOFT_TRIGGER_WIDTH: {
            if (frame->length >= 4) {
                // Expect 32-bit width in nanoseconds (little-endian)
                uint32_t requested_width = frame->data[0] | 
                                         (frame->data[1] << 8) |
                                         (frame->data[2] << 16) |
                                         (frame->data[3] << 24);
                
                // CRITICAL: Validate pulse width to prevent hardware damage
                // Pulses longer than 200ns could destroy the discharge circuit
                if (requested_width > PULSE_MAX_WIDTH_NS) {
                    uart_send_response(UART_RESP_ERROR, nullptr, 0);
                } else if (requested_width < PULSE_MIN_WIDTH_NS) {
                    uart_send_response(UART_RESP_ERROR, nullptr, 0);
                } else {
                    soft_trigger_width_ns = requested_width;
                    uart_send_response(UART_RESP_OK, nullptr, 0);
                }
            } else {
                uart_send_response(UART_RESP_ERROR, nullptr, 0);
            }
            break;
        }
            
        case UART_CMD_GET_SOFT_TRIGGER_WIDTH: {
            uint8_t width_data[4];
            width_data[0] = soft_trigger_width_ns & 0xFF;
            width_data[1] = (soft_trigger_width_ns >> 8) & 0xFF;
            width_data[2] = (soft_trigger_width_ns >> 16) & 0xFF;
            width_data[3] = (soft_trigger_width_ns >> 24) & 0xFF;
            uart_send_response(UART_RESP_OK, width_data, 4);
            break;
        }
            
        case UART_CMD_SET_VOLTAGE: {
            // Voltage setting handled on PC via calibration lookup + SET_POT_POSITION
            uart_send_response(UART_RESP_ERROR, nullptr, 0);
            break;
        }

        case UART_CMD_SET_POT_POSITION: {
            if (frame->length >= 2) {
                uint16_t position = frame->data[0] | (frame->data[1] << 8);
                bool was_charging = lt3751_is_charging();
                if (was_charging) {
                    lt3751_stop_charging();
                }
                if (position <= 1023 && ad5293_set_position(position)) {
                    if (was_charging) {
                        lt3751_start_charging();
                    }
                    uart_send_response(UART_RESP_OK, nullptr, 0);
                } else {
                    uart_send_response(UART_RESP_ERROR, nullptr, 0);
                }
            } else {
                uart_send_response(UART_RESP_ERROR, nullptr, 0);
            }
            break;
        }

        case UART_CMD_GET_POT_POSITION: {
            uint16_t position;
            if (ad5293_get_position(&position)) {
                uint8_t pos_data[2] = {(uint8_t)(position & 0xFF), (uint8_t)(position >> 8)};
                uart_send_response(UART_RESP_OK, pos_data, 2);
            } else {
                uart_send_response(UART_RESP_ERROR, nullptr, 0);
            }
            break;
        }

        case UART_CMD_GET_VOLTAGE: {
            uint8_t voltage_data[4];
            memcpy(voltage_data, &target_voltage, 4);
            uart_send_response(UART_RESP_OK, voltage_data, sizeof(voltage_data));
            break;
        }
            
        case UART_CMD_GET_VOLTAGE_MEASURED: {
            uint8_t voltage_data[4];
            memcpy(voltage_data, &current_voltage, 4);
            uart_send_response(UART_RESP_OK, voltage_data, sizeof(voltage_data));
            break;
        }

        case UART_CMD_GET_ADC_DEBUG: {
            uint8_t hi, lo;
            uint16_t raw;
            if (mcp3201_read_raw_debug(&hi, &lo, &raw)) {
                uint8_t debug_data[4] = {hi, lo, (uint8_t)(raw & 0xFF), (uint8_t)(raw >> 8)};
                uart_send_response(UART_RESP_OK, debug_data, 4);
            } else {
                uart_send_response(UART_RESP_ERROR, nullptr, 0);
            }
            break;
        }

        case UART_CMD_GET_ADC_SW: {
            uint16_t raw;
            if (mcp3201_read_raw_sw(&raw)) {
                float voltage = raw * MCP3201_LSB_SIZE_V * HV_DIVIDER_RATIO;
                uint8_t sw_data[6];
                sw_data[0] = (uint8_t)(raw & 0xFF);
                sw_data[1] = (uint8_t)(raw >> 8);
                memcpy(&sw_data[2], &voltage, 4);
                uart_send_response(UART_RESP_OK, sw_data, 6);
            } else {
                uart_send_response(UART_RESP_ERROR, nullptr, 0);
            }
            break;
        }
            
        case UART_CMD_READ_TEMPERATURES: {
            uint8_t temp_data[16];  // 4 floats × 4 bytes each
            memcpy(temp_data, current_temperatures, sizeof(current_temperatures));
            uart_send_response(UART_RESP_OK, temp_data, sizeof(temp_data));
            break;
        }
            
        default: {
            uart_send_response(UART_RESP_INVALID_CMD, nullptr, 0);
            break;
        }
    }
}

void setup() {
    uart_protocol_init(handle_uart_frame);
    system_init();
}

void loop() {
    unsigned long current_time = millis();
    uart_protocol_process();
    
    if (current_time - last_measurement_time >= MEASUREMENT_INTERVAL_MS) {
        update_measurements();
        update_led_indicators();
        last_measurement_time = current_time;
    }

    // Safety: stop charging on temperature alert or charger fault
    if (lt3751_is_charging()) {
        if (temperature_alert) {
            lt3751_stop_charging();
        }
    }
}