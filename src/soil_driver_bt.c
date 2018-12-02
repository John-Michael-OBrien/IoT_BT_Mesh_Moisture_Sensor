/*
 * @file soil_driver.c
 * @brief Provides hardware abstraction for the soil measurement
 *
 * @author John-Michael O'Brien
 * @date Nov 25, 2018
 */
/*
 * # License
 * <b>Copyright 2015 Silicon Labs, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 */

#include "stdint.h"
#include "stdbool.h"

#include "em_cmu.h"
#include "em_device.h"
#include "em_adc.h"
#include "em_gpio.h"

#include "debug.h"
#include "utils_bt.h"

#include "soil_driver_bt.h"

static void _power_on_sensor();
static void _power_off_sensor();
static void _ready();
static void _unready();

void soil_init(const uint32_t event_signal_mask) {
	/* Connect the GPIO peripheral to the HS Clock Bus */
	CMU_ClockEnable(cmuClock_GPIO, true);
	/* Configure the power pin and port to be a strong output */
	GPIO_DriveStrengthSet(SOIL_PWR_PORT, gpioDriveStrengthStrongAlternateStrong);

}

/*
 * @brief Starts and configures the ADC and associated GPIO pins.
 *
 * @return void
 */
void _ready() {
	ADC_Init_TypeDef _init_settings = ADC_INIT_DEFAULT;
	ADC_InitSingle_TypeDef _init_settings_single = ADC_INITSINGLE_DEFAULT;
	/* Connect the ADC0 peripheral to the HS Clock Bus */
	CMU_ClockEnable(cmuClock_ADC0, true);

	/* Setup the timebases and initialize the ADC */
	_init_settings.timebase = ADC_TimebaseCalc(0);
	_init_settings.prescale = ADC_PrescaleCalc(400000, 0);
	_init_settings.warmUpMode =

	/* Setup for a single ended, long duration measurement */
	_init_settings_single.acqTime = adcAcqTime256;
	_init_settings_single.diff = false;
	_init_settings_single.posSel = SOIL_SIGNAL_POS_MUX;
	_init_settings_single.negSel = SOIL_SIGNAL_NEG_MUX;
	_init_settings_single.reference = SOIL_SIGNAL_REF;

	/* Ask EM lib to set things up for us. */
	ADC_Init(ADC0, &_init_settings);
	ADC_InitSingle(ADC0, &_init_settings_single);

	return;
}

/*
 * @brief Starts up the sensor
 *
 * @return void
 */
static void _power_on_sensor() {
	/* Turn on the power management pin */
	GPIO_PinModeSet(SOIL_PWR_PORT,SOIL_PWR_PIN, gpioModePushPull, true);
}

/*
 * @brief Shuts down the sensor
 *
 * @return void
 */
static void _power_off_sensor() {
	/* Turn off the power management pin */
	GPIO_PinModeSet(SOIL_PWR_PORT,SOIL_PWR_PIN, gpioModeDisabled, false);
}

/*
 * @brief Shuts down the ADC
 *
 * @return void
 */
void _unready() {
	/* Reset/disable the ADC */
	ADC_Reset(ADC0);
	/* And unclock it */
	CMU_ClockEnable(cmuClock_ADC0, false);
}

/*
 * @brief Synchronously starts the sensor, takes a measurement, and shuts down the sensor.
 *
 * Does not require the BGAPI to function but does not provide power on delays.
 *
 * @return The ADC result of the soil reading
 */
uint16_t soil_get_reading_sync() {
	uint16_t result;

	/* Turn on the sensor */
	_power_on_sensor();
	/* and ready the ADC */
	_ready();

	/* Start the measurement */
	ADC_Start(ADC0, adcStartSingle);
	/* Stall for the measurement to finish */
	while ( (ADC0->STATUS & ADC_STATUS_SINGLEDV) == 0 ) {
	}
	/* Cache the result */
	result = ADC_DataSingleGet(ADC0);

	/* And shut down the ADC */
	_unready();
	/* Turn off the sensor */
	_power_off_sensor();

	/* And return the result. */
	return result;
}

/*
 * @brief Starts the power on process for the ADC
 *
 * Requires the BGAPI be initialized and that a handler for the external signal be in place.
 *
 * @return The ADC result of the soil reading
 */
void soil_start_reading_async() {
	/* Turn on the sensor */
	_power_on_sensor();
	/* And start the delay */
	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_hardware_set_soft_timer(GET_SOFT_TIMER_COUNTS(SOIL_POWER_ON_TIME), SOIL_POWER_ON_HANDLE, SOFT_TIMER_ONE_SHOT)->result,
			"Failed to start sensor power on timer.");
}

/*
 * @brief Finishes the power on and makes the measurement.
 *
 * Should be called from the BGAPI external signal handler.
 *
 * @return The ADC result of the soil reading
 */
uint16_t soil_finish_reading_async() {
	uint16_t result;


	/* Ready the ADC */
	_ready();

	/* Start the measurement */
	ADC_Start(ADC0, adcStartSingle);
	/* Stall for the measurement to finish */
	while ( (ADC0->STATUS & ADC_STATUS_SINGLEDV) == 0 ) {
	}
	/* Cache the result */
	result = ADC_DataSingleGet(ADC0);

	/* And shut down the ADC */
	_unready();
	/* Turn off the sensor */
	_power_off_sensor();

	/* And return the result. */
	return result;
}
