/*
 * @file adc_driver.c
 * @brief Provides hardware abstraction for the ADC
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

#include "stdbool.h"

#include "em_cmu.h"
#include "em_device.h"
#include "em_adc.h"

// TODO: Documentation
void adc_init() {
	ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
	ADC_InitSingle_TypeDef init_single = ADC_INITSINGLE_DEFAULT;

	/* Setup the timebases and initialize the ADC */
	init.timebase = ADC_TimebaseCalc(0);
	init.prescale = ADC_PrescaleCalc(400000, 0);

	init_single.acqTime = adcAcqTime256;
	init_single.diff = false;
	init_single.posSel = adcPosSelAPORT1XCH10;
	//init_single.negSel = adcNegSelAPORT1YCH11;
	init_single.negSel = adcNegSelVSS;
	init_single.reference = adcRefVDD;

	/* Connect the ADC0 peripheral to the HS Clock Bus */
	CMU_ClockEnable(cmuClock_ADC0, true);

	/* Ask EM lib to set things up for us. */
	ADC_Init(ADC0, &init);
	ADC_InitSingle(ADC0, &init_single);
}

// TODO: Documentation
uint16_t adc_get_reading_sync() {
	ADC_Start(ADC0, adcStartSingle);
	while ( (ADC0->STATUS & ADC_STATUS_SINGLEDV) == 0 ) {
	}
	return ADC_DataSingleGet(ADC0);
}
