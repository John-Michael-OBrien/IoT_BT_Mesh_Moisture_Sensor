/*
 * @file led_driver.c
 * @brief Provides an API for working driving LED1 on the BRD4001.
 *
 * @author John-Michael O'Brien
 * @date Sep 11, 2018
 */

#ifndef SRC_LED_DRIVER_H_
#define SRC_LED_DRIVER_H_

#include "em_gpio.h"

#define LED_PORT (gpioPortF) /* LED1 on BRD4001A */
#define LED_PIN (5) /* LED1 on BRD4001A */

void led_init(void);
void led_on(void);
void led_off(void);

#endif /* SRC_LED_DRIVER_H_ */
