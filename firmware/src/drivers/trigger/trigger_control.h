#ifndef TRIGGER_CONTROL_H
#define TRIGGER_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include "../../core/config/pin_config.h"

/**
 * @brief Initialize trigger control system
 * @return true on success, false on failure
 */
void trigger_control_init(void);

/**
 * @brief Enable trigger system
 * @details Allows external/internal triggers to pass through to pulse generation
 * @return true on success
 */
void trigger_enable(void);

/**
 * @brief Disable trigger system
 * @details Blocks all triggers from generating pulses (safety gate)
 */
void trigger_disable(void);

/**
 * @brief Check if trigger system is enabled
 * @return true if trigger enable is asserted
 */
bool is_trigger_enabled(void);

#endif // TRIGGER_CONTROL_H