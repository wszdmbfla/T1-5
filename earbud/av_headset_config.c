/*!
\copyright  Copyright (c) 2018 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    Part of 6.3.2
\file       av_headset_config.c
\brief	    Config data
*/

#include <panic.h>

#include "av_headset_config.h"
#include "av_headset_log.h"

#if defined(INCLUDE_PROXIMITY)

#define PROXIMITY_PIO_ON        255
#define PROXIMITY_PIO_I2C_SCL   2
#define PROXIMITY_PIO_I2C_SDA   3
#define PROXIMITY_PIO_INT       4

#if   defined(HAVE_VNCL3020)

#include "peripherals/vncl3020.h"
const struct __proximity_config proximity_config = {
    .threshold_low = 3000,
    .threshold_high = 3500,
    .threshold_counts = threshold_count_4,
    .rate = proximity_rate_7p8125_per_second,
    .i2c_clock_khz = 100,
    .pios = {
        /* The PROXIMITY_PIO definitions are defined in the platform x2p file */
        .on = PROXIMITY_PIO_ON,
        .i2c_scl = PROXIMITY_PIO_I2C_SCL,
        .i2c_sda = PROXIMITY_PIO_I2C_SDA,
        .interrupt = PROXIMITY_PIO_INT,
    },
};
#elif defined(HAVE_TXC_PA22X)

#include "peripherals/txc_pa22x.h"

struct __proximity_config proximity_config = {
    .threshold_low = 90,//65,//45,
    .threshold_high = 110,//75,//55,
    .threshold_counts = threshold_count_4,
    .rate = proximity_rate_7p8125_per_second,
    .i2c_clock_khz = 100,
    .pios = {
        /* The PROXIMITY_PIO definitions are defined in the platform x2p file */
        .on = PROXIMITY_PIO_ON,
        .i2c_scl = PROXIMITY_PIO_I2C_SCL,
        .i2c_sda = PROXIMITY_PIO_I2C_SDA,
        .interrupt = PROXIMITY_PIO_INT,
    },
};
#elif defined(HAVE_LTR2668)

#include "peripherals/ltr2668.h"
//#include "peripherals/proximity_private.h"

const struct __proximity_config proximity_config = {
    .threshold_low = 180, //3000,
    .threshold_high = 220,
    .threshold_counts = 0, //ltr2668_threshold_count_0,
    .rate = ltr2668_proximity_rate_25_ms,
    .i2c_clock_khz = 100,
    .pios = {
        /* The PROXIMITY_PIO definitions are defined in the platform x2p file */
        .on = PROXIMITY_PIO_ON,
        .i2c_scl = PROXIMITY_PIO_I2C_SCL,
        .i2c_sda = PROXIMITY_PIO_I2C_SDA,
        .interrupt = PROXIMITY_PIO_INT,
    },
};
#else
#error INCLUDE_PROXIMITY was defined, but no proximity sensor type was defined.
#endif   /* HAVE_VNCL3020 */
#endif /* INCLUDE_PROXIMITY */

#if defined(INCLUDE_TAP_SENSOR)
#define TAP_SENSOR_PIO_ON        255
#define TAP_SENSOR_PIO_I2C_SCL   2
#define TAP_SENSOR_PIO_I2C_SDA   3
#define TAP_SENSOR_PIO_INT       5

#if defined(HAVE_DA230)
#include "peripherals/da230.h"
const struct __tap_sensor_config tap_sensor_config = {
    .i2c_clock_khz = 100,
    .pios = {
        /* The TAP_SENSOR_PIO definitions are defined in the platform x2p file */
        .on = TAP_SENSOR_PIO_ON,
        .i2c_scl = TAP_SENSOR_PIO_I2C_SCL,
        .i2c_sda = TAP_SENSOR_PIO_I2C_SDA,
        .interrupt = TAP_SENSOR_PIO_INT,
    },
};
#else
#error INCLUDE_TAP_SENSOR was defined, but no tap sensor type was defined.
#endif
#endif

#if defined(INCLUDE_ACCELEROMETER)
#if   defined(HAVE_ADXL362)

#include "peripherals/adxl362.h"
const struct __accelerometer_config accelerometer_config = {
    /* 250mg activity threshold, magic value from datasheet */
    .activity_threshold = 0x00FA,
    /* 150mg activity threshold, magic value from datasheet */
    .inactivity_threshold = 0x0096,
    /* Inactivity timer is about 5 seconds */
    .inactivity_timer = 30,
    .spi_clock_khz = 400,
    .pios = {
        /* The ACCELEROMETER_PIO definitions are defined in the platform x2p file */
        .on = ACCELEROMETER_PIO_ON,
        .spi_clk = ACCELEROMETER_PIO_SPI_CLK,
        .spi_cs = ACCELEROMETER_PIO_SPI_CS,
        .spi_mosi = ACCELEROMETER_PIO_SPI_MOSI,
        .spi_miso = ACCELEROMETER_PIO_SPI_MISO,
        .interrupt = ACCELEROMETER_PIO_INT,
    },
};
#else
#error INCLUDE_ACCELEROMETER was defined, but no accelerometer type was defined.
#endif   /* HAVE_ADXL362*/
#endif /* INCLUDE_ACCELEROMETER */

#if defined(INCLUDE_TEMPERATURE)
#if   defined(HAVE_THERMISTOR)

#include "peripherals/thermistor.h"
/* Requrired for THERMISTOR_ADC */
#include <app/adc/adc_if.h>
const thermistorConfig thermistor_config = {
    .on = THERMISTOR_ON,
    .adc = THERMISTOR_ADC,
};
#else
#error INCLUDE_TEMPERATURE was defined, but no temperature sensor was defined.
#endif   /* HAVE_THERMISTOR */
#endif /* INCLUDE_TEMPERATURE */

/*! Configuration of all audio prompts - a configuration  must be defined for
each of the NUMBER_OF_PROMPTS in the system. */
const promptConfig prompt_config[] =
{
    [PROMPT_POWER_ON] = {
        .filename = "power_on.prm",
        .rate = 8000,
        .format = PROMPT_FORMAT_PCM,
    },
    [PROMPT_POWER_OFF] = {
        .filename = "power_off.prm",
        .rate = 8000,
        .format = PROMPT_FORMAT_PCM,
    },
    [PROMPT_PAIRING] = {
        .filename = "pairing.prm",
        .rate = 8000,
        .format = PROMPT_FORMAT_PCM,
    },
    [PROMPT_PAIRING_SUCCESSFUL] = {
        .filename = "pairing_successful.prm",
        .rate = 8000,
        .format = PROMPT_FORMAT_PCM,
    },
    [PROMPT_PAIRING_FAILED] = {
        .filename = "pairing_failed.prm",
        .rate = 8000,
        .format = PROMPT_FORMAT_PCM,
    },
    [PROMPT_CONNECTED] = {
        .filename = "connected.prm",
        .rate = 8000,
        .format = PROMPT_FORMAT_PCM,
    },
    [PROMPT_DISCONNECTED] = {
        .filename = "disconnected.prm",
        .rate = 8000,
        .format = PROMPT_FORMAT_PCM,
    },
    [PROMPT_LOW_BATTERY] = {
        .filename = "battery_low.prm",
        .rate = 8000,
        .format = PROMPT_FORMAT_PCM,
    },
    [PROMPT_DOUBLE] = {
        .filename = "double.prm",
        .rate = 8000,
        .format = PROMPT_FORMAT_PCM,
    },
    [PROMPT_DUT] = {
        .filename = "dut.prm",
        .rate = 8000,
        .format = PROMPT_FORMAT_PCM,
    },
};

/*! A prompt configuration must be defined for each prompt. */
COMPILE_TIME_ASSERT(ARRAY_DIM(prompt_config) == NUMBER_OF_PROMPTS, missing_prompt_configurations);


bool appConfigBleGetAdvertisingRate(appConfigBleAdvertisingMode mode, uint16 *min_rate, uint16 *max_rate)
{
    if (min_rate && max_rate)
    {
        switch (mode)
        {
            case APP_ADVERT_RATE_SLOW:
                *min_rate = appConfigBleSlowAdvertisingRateMin();
                *max_rate = appConfigBleSlowAdvertisingRateMax();
                break;

            case APP_ADVERT_RATE_FAST:
                *min_rate = appConfigBleFastAdvertisingRateMin();
                *max_rate = appConfigBleFastAdvertisingRateMax();
                break;

            default:
                DEBUG_LOG("appConfigBleGetAdvertisingRate Unsupported mode:%d requested",mode);
                Panic();
        }

        DEBUG_LOG("appConfigBleGetAdvertisingRate. Mode:%d requested. Adv rate %d-%d",
                                            mode, *min_rate, *max_rate);
        return TRUE;
    }
    return FALSE;
}

