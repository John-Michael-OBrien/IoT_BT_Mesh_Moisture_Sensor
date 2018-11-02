/*
 * @file pb_driver_bt.h
 * @brief Provides an API for bluetooth stack based pushbutton detection.
 *
 * Consumes the Odd GPIO Interrupt resource
 *
 * @author John-Michael O'Brien
 * @date Oct 3, 2018
 */

#ifndef SRC_PB_DRIVER_BT_H_
#define SRC_PB_DRIVER_BT_H_

#include "em_gpio.h"
#include "stdbool.h"


#define PB0_PORT (gpioPortF) /* PB0 on BRD4001A */
#define PB0_PIN (6) /* PB0 on BRD4001A */
#define PB0_INT (5) /* Must be an unused ODD interrupt within the bank used the selected pin (for pin 6 this means only 5 and 7 are available.) */

#define PB1_PORT (gpioPortF) /* PB1 on BRD4001A */
#define PB1_PIN (7) /* PB1 on BRD4001A */
#define PB1_INT (7) /* Must be an unused ODD interrupt within the bank used the selected pin (for pin 6 this means only 5 and 7 are available.) */

#define _PB0_INT_MASK ((1<<PB0_INT) << _GPIO_IF_EXT_SHIFT)
#define _PB1_INT_MASK ((1<<PB1_INT) << _GPIO_IF_EXT_SHIFT)

void pb_init();
void pb_start();
void pb_stop();
bool pb_get_pb0();
bool pb_get_pb1();

#endif /* SRC_PB_DRIVER_BT_H_ */
