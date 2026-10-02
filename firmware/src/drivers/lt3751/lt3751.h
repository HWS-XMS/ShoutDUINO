#ifndef LT3751_H
    #define LT3751_H

    #include <stdint.h>
    #include <stdbool.h>
    #include "../../core/config/pin_config.h"

    /**
    * @brief Initialize LT3751 controller driver
    * @return true on success, false on failure
    */
    void lt3751_init(void);

    /**
    * @brief Start charging operation
    * @return true on success, false if fault detected
    */
    bool lt3751_start_charging(void);

    /**
    * @brief Stop charging operation
    */
    void lt3751_stop_charging(void);

    /**
    * @brief Check if currently charging
    * @return true if CHARGE pin is asserted
    */
    bool lt3751_is_charging(void);


    /**
    * @brief Check for fault condition
    * @return true if nFAULT is asserted (fault detected)
    */
    bool lt3751_has_fault(void);

    /**
    * @brief Check if charging is complete
    * @return true if nDONE is asserted (charging complete)
    * @note In feedback auto-regulated mode, this should typically not occur
    */
    bool lt3751_is_done(void);

    /**
    * @brief Clear fault condition (if possible)
    * @details Briefly toggles CHARGE pin to attempt fault recovery
    * @return true if fault cleared, false if fault persists
    */
    bool lt3751_clear_fault(void);

#endif // LT3751_H