#ifndef AD5293_H
    #define AD5293_H

    #include <stdint.h>
    #include <stdbool.h>
    #include "../../core/config/pin_config.h"

    // =============================================================================
    // AD5293 Configuration
    // =============================================================================

    // AD5293 specifications
    #define AD5293_MAX_POSITION 1023        // 10-bit resolution (0-1023)
    #define AD5293_RESISTANCE   20000       // 20kΩ nominal resistance

    // =============================================================================
    // AD5293 Commands
    // =============================================================================

    typedef enum {
        AD5293_CMD_NOP          = 0x0,     // Command 0: 0000
        AD5293_CMD_WRITE_RDAC   = 0x1,     // Command 1: 0001
        AD5293_CMD_READ_RDAC    = 0x2,     // Command 2: 0010
        AD5293_CMD_SW_RESET     = 0x4,     // Command 3: 0100
        AD5293_CMD_WRITE_CTRL   = 0x6,     // Command 4: 0110
        AD5293_CMD_READ_CTRL    = 0x7,     // Command 5: 0111
        AD5293_CMD_SHUTDOWN     = 0x8      // Command 6: 1000
    } ad5293_command_t;

    // =============================================================================
    // AD5293 API
    // =============================================================================

    /**
    * @brief Initialize AD5293 digital potentiometer
    */
    void ad5293_init(void);

    /**
    * @brief Set wiper position (0-1023)
    * @param position Wiper position (0 = minimum resistance, 1023 = maximum)
    * @return true on success, false on failure
    */
    bool ad5293_set_position(uint16_t position);

    /**
    * @brief Get current wiper position
    * @param position Pointer to store current wiper position
    * @return true on success, false on failure
    */
    bool ad5293_get_position(uint16_t *position);

    /**
    * @brief Set resistance value
    * @param resistance_ohms Target resistance in ohms (0 to AD5293_RESISTANCE)
    * @return true on success, false on failure
    */
    bool ad5293_set_resistance(uint16_t resistance_ohms);

    /**
    * @brief Get current resistance value
    * @param resistance_ohms Pointer to store current resistance in ohms
    * @return true on success, false on failure
    */
    bool ad5293_get_resistance(uint16_t *resistance_ohms);

    /**
    * @brief Perform software reset
    * @return true on success, false on failure
    */
    bool ad5293_reset(void);

#endif