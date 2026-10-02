/**
 * @file gate_driver.h
 * @brief Gate driver control for discharge MOSFET
 * @details Controls the gate driver (U10) that drives the discharge MOSFET (NTBG015N065SC1)
 */

#ifndef GATE_DRIVER_H
#define GATE_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "../../core/config/pin_config.h"

/**
 * @brief Initialize gate driver controller
 * @return true on success, false on failure
 */
void gate_driver_init(void);

/**
 * @brief Enable gate driver
 * @details Allows gate driver to respond to pulse signals
 * @return true on success
 */
void gate_driver_enable(void);

/**
 * @brief Disable gate driver
 * @details Prevents gate driver from driving the MOSFET
 */
void gate_driver_disable(void);

/**
 * @brief Check if gate driver is enabled
 * @return true if gate driver is enabled
 */
bool is_gate_driver_enabled(void);

#endif // GATE_DRIVER_H