/**
Pin Assignment Summary:
    Digital Pins:
    Pin 3  - LT3751_nFAULT    (Input, active low)
    Pin 4  - LT3751_nDONE     (Input, active low)
    Pin 5  - INTERNAL_TRIGGER (Output, pulse generation)
    Pin 6  - TRIGGER_ENABLE   (Output, active high)
    Pin 7  - GATEDRIVER_EN    (Output, active high)
    Pin 8  - CHARGE           (Output, active high)
    Pin 9  - POT_nCS          (Output, SPI chip select)
    Pin 10 - ADC_nCS          (Output, SPI chip select)
    Pin 11 - MOSI             (Output, SPI data)
    Pin 12 - MISO             (Input, SPI data)
    Pin 13 - SCK              (Output, SPI clock)

    Analog Pins:
    A0     - TEMP_ALERT       (Input, active high)
    A1     - POT_READY        (Input, active high)
    A2     - HT_MEASURED      (Output, LED control)
    A3     - HV_MEASURED      (Output, LED control)
    A4     - SDA              (I2C data, bidirectional)
    A5     - SCL              (I2C clock, output)
*/

#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// Digital Pins
#define LT3751_nFAULT_PIN       3
#define LT3751_nDONE_PIN        4
#define PULSE_GENERATOR_PIN     5       // INTERNAL_TRIGGER
#define TRIGGER_ENABLE_PIN      6
#define GATE_DRIVER_EN_PIN      7       // GATEDRIVER_EN
#define LT3751_CHARGE_PIN       8       // CHARGE
#define AD5293_CS_PIN           9       // POT_nCS
#define MCP3201_CS_PIN          10      // ADC_nCS
#define SPI_MOSI_PIN            11
#define SPI_MISO_PIN            12
#define SPI_SCK_PIN             13      // SPI_CLK

// Analog Pins
#define TEMP_ALERT_PIN          A0
#define POT_READY_PIN           A1
#define LED_HV_MEASURED_PIN     A2      // HV_MEASURED (Voltage > 10V)
#define LED_HT_MEASURED_PIN     A3      // HT_MEASURED (Temperature high)
#define I2C_SDA_PIN             A4      // TEMP_SDA
#define I2C_SCL_PIN             A5      // TEMP_SCL

#endif // PIN_CONFIG_H