/*
 * @file led_driver.c
 * @brief Provides an API for working driving LED1 on the BRD4001.
 *
 * @author John-Michael O'Brien
 * @date Sep 11, 2018
 */

#include "stdbool.h"

#include "em_chip.h"
#include "em_device.h"
#include "em_gpio.h"
#include "em_cmu.h"

#include "led_driver.h"


/*
 * @brief Initializes the LED driver.
 *
 * @return void
*/
void led_init() {
	/* Connect the GPIO peripheral to the HS Clock Bus */
	CMU_ClockEnable(cmuClock_GPIO, true);

	/* Configure the LED pin and port to be a weak output */
	GPIO_PinModeSet(LED_PORT,LED_PIN, gpioModePushPull, false);
	GPIO_DriveStrengthSet(LED_PORT, gpioDriveStrengthWeakAlternateWeak);
	return;
}

/*
 * @brief Turns the LED on
 *
 * @return void
*/
void led_on() {
	/* Turn on the LED by setting the GPIO */
	GPIO_PinOutSet(LED_PORT,LED_PIN);
}

/*
 * @brief Turns the LED off
 *
 * @return void
*/
void led_off() {
	/* Turn off the LED by clearing the GPIO */
	GPIO_PinOutClear(LED_PORT,LED_PIN);
}
