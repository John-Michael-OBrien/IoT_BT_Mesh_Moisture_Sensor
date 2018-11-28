/*
 * @file adc_driver.h
 * @brief Provides hardware abstraction for the ADC
 *
 * @author John-Michael O'Brien
 * @date Nov 25, 2018
 */

#ifndef SRC_ADC_DRIVER_H_
#define SRC_ADC_DRIVER_H_

void adc_init();
uint16_t adc_get_reading_sync();

#endif /* SRC_ADC_DRIVER_H_ */
