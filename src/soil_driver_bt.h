/*
 * @file soil_driver.h
 * @brief Provides hardware abstraction for the soil measurement
 *
 * @author John-Michael O'Brien
 * @date Nov 25, 2018
 */

#ifndef SRC_SOIL_DRIVER_BT_H_
#define SRC_SOIL_DRIVER_BT_H_

#include "stdint.h"

#define SOIL_PWR_PORT (gpioPortD)
#define SOIL_PWR_PIN (10)

#define SOIL_POS_PORT (gpioPortD)
#define SOIL_POS_PIN (12)

#define SOIL_NEG_PORT (gpioPortD)
#define SOIL_NEG_PIN (11)

/* Wait 10ms for the sensor to level out. */
#define SOIL_POWER_ON_TIME (0.010) /* s */

//#define SOIL_SIGNAL_POS_MUX (adcPosSelAPORT1XCH10)
#define SOIL_SIGNAL_POS_MUX (adcPosSelAPORT4XCH3)
#define SOIL_SIGNAL_NEG_MUX (adcPosSelAPORT4YCH4)
//#define SOIL_SIGNAL_NEG_MUX (adcNegSelVSS)
#define SOIL_SIGNAL_REF (adcRefVDD)


void soil_init();
uint16_t soil_get_reading_sync();
void soil_start_reading_async();
uint16_t soil_finish_reading_async();

#endif /* SRC_SOIL_DRIVER_BT_H_ */
