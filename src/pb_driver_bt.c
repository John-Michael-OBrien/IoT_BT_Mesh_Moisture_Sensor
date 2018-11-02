/*
 * @file pb_driver_bt.c
 * @brief Provides an API for bluetooth stack based pushbutton detection.
 *
 * Consumes the Odd GPIO Interrupt resource
 *
 * @author John-Michael O'Brien
 * @date Oct 3, 2018
 */


#ifndef SRC_PB_DRIVER_C_
#define SRC_PB_DRIVER_C_

#include "em_chip.h"
#include "em_device.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "native_gecko.h"
#include <sleep.h>
#include <src/pb_driver_bt.h>
#include "user_signals_bt.h"
#include "stdbool.h"


/*
 * @brief Prepares the pushbutton driver for operation. Should be called before calling any other pb routine.
 *
 * @param pb0_mask The event mask to use with the bluetooth stack for PB0
 * @param pb1_mask The event mask to use with the bluetooth stack for PB1
 *
 * @return void
 */
void pb_init(uint32_t pb0_mask, uint32_t pb1_mask) {
	/* Connect the GPIO peripheral to the HS Clock Bus */
	CMU_ClockEnable(cmuClock_GPIO, true);

	/* Configure the PB pins and to be inputs */
	GPIO_PinModeSet(PB0_PORT,PB0_PIN, gpioModeInput, false);
	GPIO_PinModeSet(PB1_PORT,PB1_PIN, gpioModeInput, false);

	/* Configure our interrupts for the falling edge (button is negative logic) */
	GPIO_ExtIntConfig(PB0_PORT,PB0_PIN,PB0_INT, false, true, true);
	GPIO_ExtIntConfig(PB1_PORT,PB1_PIN,PB1_INT, false, true, true);

	return;
}

/*
 * @brief Starts generating BT stack events when pushbuttons are used.
 *
 * Must be called after pb_init and also after (or during) evt_system_boot
 *
 * @return void
 */
void pb_start() {
	/* Clear any pending interrupts */
	GPIO_IntClear(_PB0_INT_MASK | _PB1_INT_MASK);
	/* Enable the interrupt */
	GPIO_IntEnable(_PB0_INT_MASK | _PB1_INT_MASK);

	/* Block sleeping below EM3 (Not usually an issue, but we're not a wake on EM4 driver) */
	SLEEP_SleepBlockBegin(sleepEM3);

	/* Finally, enable the interrupt to the CPU */
	NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

/*
 * @brief Stops generating BT stack events when pushbuttons are used.
 *
 * Must be called after pb_init and also after (or during) evt_system_boot.
 * Also, be aware that if an interrupt occurs during the call jump, you may still
 * end up with one more event.
1 *
 * @return void
 */
void pb_stop() {
	/* Disable the interrupt to the CPU */
	NVIC_DisableIRQ(GPIO_ODD_IRQn);

	/* Disable the interrupt */
	GPIO_IntDisable(_PB0_INT_MASK | _PB1_INT_MASK);
	/* Clear any pending interrupts */
	GPIO_IntClear(_PB0_INT_MASK | _PB1_INT_MASK);

	/* Remove our sleep block (Want EM4? Go for it!) */
	SLEEP_SleepBlockEnd(sleepEM3);
}

/* TODO: Documentation */
bool pb_get_pb0() {
	if (GPIO_PinInGet(PB0_PORT, PB0_PIN)) {
		return false;
	} else {
		return true;
	}
}

/* TODO: Documentation */
bool pb_get_pb1() {
	if (GPIO_PinInGet(PB1_PORT, PB1_PIN)) {
		return false;
	} else {
		return true;
	}
}

/*
 * @brief Interrupt handler for the ODD IRQs
 *
 * @return void
 */
void GPIO_ODD_IRQHandler() {
	/* If we fired on PB0 rise (we don't do fall) */
	if (GPIO_IntGet() & _PB0_INT_MASK) {
		/* Clear the interrupt quickly so we can retrigger */
		GPIO_IntClear(_PB0_INT_MASK);
		/* and tell the main program about it. */
		gecko_external_signal(PB_EVT_0);
	}
	/* If we fired on PB1 rise (we don't do fall) */
	if (GPIO_IntGet() & _PB1_INT_MASK) {
		/* Clear the interrupt quickly so we can retrigger */
		GPIO_IntClear(_PB1_INT_MASK);
		/* and tell the main program about it. */
		gecko_external_signal(PB_EVT_1);
	}
}

#endif /* SRC_PB_DRIVER_C_ */
