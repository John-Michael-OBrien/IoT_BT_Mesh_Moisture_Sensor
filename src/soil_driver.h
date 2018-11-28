/*
 * @file soil_driver.h
 * @brief Provides hardware abstraction for the soil measurement
 *
 * @author John-Michael O'Brien
 * @date Nov 25, 2018
 */

#ifndef SRC_SOIL_DRIVER_H_
#define SRC_SOIL_DRIVER_H_

#define SOIL_PWR_PORT (gpioPortD)
#define SOIL_PWR_PIN (10)

#define SOIL_POS_PORT (gpioPortD)
#define SOIL_POS_PIN (12)

#define SOIL_NEG_PORT (gpioPortD)
#define SOIL_NEG_PIN (11)

//#define SOIL_SIGNAL_POS_MUX (adcPosSelAPORT1XCH10)
#define SOIL_SIGNAL_POS_MUX (adcPosSelAPORT4XCH3)
#define SOIL_SIGNAL_NEG_MUX (adcPosSelAPORT4YCH4)
//#define SOIL_SIGNAL_NEG_MUX (adcNegSelVSS)
#define SOIL_SIGNAL_REF (adcRefVDD)


void soil_init();
void soil_deinit();
uint16_t soil_get_reading_sync();

#endif /* SRC_SOIL_DRIVER_H_ */
