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

#include <src/soil_driver_bt.h>
#include "stdbool.h"

#include "em_cmu.h"
#include "em_device.h"
#include "em_adc.h"
#include "em_gpio.h"

#include "hf_one_shot_timer_driver_bt.h"

static void _power_on_sensor();
static void _power_off_sensor();
static void _ready();
static void _unready();

static TIMEROS_Delay_TypeDef _delay;

void soil_init(const uint32_t event_signal_mask) {
	/* Connect the GPIO peripheral to the HS Clock Bus */
	CMU_ClockEnable(cmuClock_GPIO, true);
	/* Configure the power pin and port to be a strong output */
	GPIO_DriveStrengthSet(SOIL_PWR_PORT, gpioDriveStrengthStrongAlternateStrong);

	/* Power up the timer. */
	timeros_init(event_signal_mask);
	/* And precompute our delay */
	_delay = timeros_calc_ticks(SOIL_POWER_ON_TIME);
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

	/* Ask EM lib to set things
	 *  up for us. */
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
	GPIO_PinModeSet(SOIL_PWR_PORT,SOIL_PWR_PIN, gpioModePushPull, false);
	GPIO_PinOutSet(SOIL_PWR_PORT,SOIL_PWR_PIN);
}

/*
 * @brief Shuts down the sensor
 *
 * @return void
 */
static void _power_off_sensor() {
	/* Turn off the power management pin */
	GPIO_PinOutClear(SOIL_PWR_PORT,SOIL_PWR_PIN);
	GPIO_PinModeSet(SOIL_PWR_PORT,SOIL_PWR_PIN, gpioModeDisabled, false);
}

/*
 * @brief Shuts down the ADC
 *
 * @return void
 */
void _unready() {
	ADC_Reset(ADC0);
	CMU_ClockEnable(cmuClock_ADC0, false);
}

/*
 * @brief Synchronously starts the sensor, takes a measurement, and shuts down the sensor.
 *
 * @return The ADC result of the soil reading
 */
uint16_t soil_get_reading_sync() {
	uint16_t result;

	_power_on_sensor();
	_ready();
	ADC_Start(ADC0, adcStartSingle);
	while ( (ADC0->STATUS & ADC_STATUS_SINGLEDV) == 0 ) {
	}
	result = ADC_DataSingleGet(ADC0);
	_unready();
	_power_off_sensor();
	return result;
}

void soil_start_reading_async() {
	_power_on_sensor();
	timeros_do_shot(&_delay);
}

uint16_t soil_finish_reading_async() {
	uint16_t result;

	timeros_finish_shot();
	_ready();
	ADC_Start(ADC0, adcStartSingle);
	while ( (ADC0->STATUS & ADC_STATUS_SINGLEDV) == 0 ) {
	}
	result = ADC_DataSingleGet(ADC0);
	_unready();
	_power_off_sensor();
	return result;
}