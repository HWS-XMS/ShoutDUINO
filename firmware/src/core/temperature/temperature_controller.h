#ifndef TEMPERATURE_CONTROLLER_H

    #define TEMPERATURE_CONTROLLER_H

    #include <stdint.h>
    #include <stdbool.h>

    // TMP117 I2C addresses
    #define TMP117_U1_ADDRESS    0x4A
    #define TMP117_U2_ADDRESS    0x48
    #define TMP117_U3_ADDRESS    0x4B
    #define TMP117_U14_ADDRESS   0x49

    typedef enum {
        TEMP_SENSOR_PRIMARY_NMOS    = 0,
        TEMP_SENSOR_GATE_DRIVER     = 1,
        TEMP_SENSOR_TRANSFORMER     = 2,
        TEMP_SENSOR_DISCHARGE_NMOS  = 3,
        TEMP_SENSOR_COUNT           = 4
    } temp_sensor_t;

    // Temperature controller error codes
    typedef enum {
        TEMP_ERR_OK                             = 0x00,

        // Primary NMOS sensor errors (0x01-0x03)
        TEMP_ERR_PRIMARY_NMOS_INIT              = 0x01,
        TEMP_ERR_PRIMARY_NMOS_SET_ALERT         = 0x02,
        TEMP_ERR_PRIMARY_NMOS_SET_POLARITY      = 0x03,

        // Gate driver sensor errors (0x04-0x06)
        TEMP_ERR_GATE_DRIVER_INIT               = 0x04,
        TEMP_ERR_GATE_DRIVER_SET_ALERT          = 0x05,
        TEMP_ERR_GATE_DRIVER_SET_POLARITY       = 0x06,

        // Transformer sensor errors (0x07-0x09)
        TEMP_ERR_TRANSFORMER_INIT               = 0x07,
        TEMP_ERR_TRANSFORMER_SET_ALERT          = 0x08,
        TEMP_ERR_TRANSFORMER_SET_POLARITY       = 0x09,

        // Discharge NMOS sensor errors (0x0A-0x0C)
        TEMP_ERR_DISCHARGE_NMOS_INIT            = 0x0A,
        TEMP_ERR_DISCHARGE_NMOS_SET_ALERT       = 0x0B,
        TEMP_ERR_DISCHARGE_NMOS_SET_POLARITY    = 0x0C
    } temp_error_code_t;

    // Temperature thresholds in Celsius
    #define TEMP_THRESHOLD_PRIMARY_NMOS    80.0f
    #define TEMP_THRESHOLD_GATE_DRIVER     80.0f  
    #define TEMP_THRESHOLD_TRANSFORMER     80.0f
    #define TEMP_THRESHOLD_DISCHARGE_NMOS  80.0f

    temp_error_code_t temperature_controller_init    (void);
    bool temperature_read_all           (float temperatures[TEMP_SENSOR_COUNT]);
    bool temperature_check_alert        (void);
    bool temperature_set_thresholds     (float thresholds[TEMP_SENSOR_COUNT]);

#endif