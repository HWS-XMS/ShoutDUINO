#ifndef TMP117_H
#define TMP117_H

#include <stdint.h>
#include <stdbool.h>

// Essential registers
#define TMP117_REG_TEMP_RESULT      0x00
#define TMP117_REG_CONFIGURATION    0x01
#define TMP117_REG_THIGH_LIMIT      0x02
#define TMP117_REG_TLOW_LIMIT       0x03
#define TMP117_REG_DEVICE_ID        0x0F

// Configuration bits
#define TMP117_CFG_HIGH_ALERT       (1 << 15)
#define TMP117_CFG_LOW_ALERT        (1 << 14)
#define TMP117_CFG_POL              (1 << 3)    // Alert polarity: 0=active low, 1=active high

// Constants
#define TMP117_DEVICE_ID_VALUE      0x0117
#define TMP117_RESOLUTION           0.0078125f
#define TMP117_CFG_DEFAULT          0x0220      // Continuous conversion, 1s, 8 averages

// Essential API
bool tmp117_init                (uint8_t i2c_address);
bool tmp117_read_temperature    (uint8_t i2c_address, float *temperature_c);
bool tmp117_set_alert_high      (uint8_t i2c_address, float temp_high_c);
bool tmp117_set_alert_polarity  (uint8_t i2c_address, bool active_high);
bool tmp117_is_alert_active     (uint8_t i2c_address);

#endif // TMP117_H