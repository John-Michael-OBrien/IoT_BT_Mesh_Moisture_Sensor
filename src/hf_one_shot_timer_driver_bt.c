/*
 * @file hf_one_shot_timer_driver.c
 * @brief Provides an API timing out brief intervals via callbacks
 *
 * Utilizes the TIMER0 resource.
 *
 * @author John-Michael O'Brien
 * @date Sep 20, 2018
 */

#include <stdbool.h>
#include <stdint.h>
#include "native_gecko.h"


#include <em_timer.h>
#include <em_cmu.h>
#include <em_core.h>
#include <sleep.h>

#include "debug.h"

#include "hf_one_shot_timer_driver_bt.h"

bool running = false;
uint32_t timer_signal_mask=0;


/*
 * @brief Initializes the one shot timer
 *
 * This should be called once during initialization to get everything ready,
 *
 * @param event_signal_mask The mask to provide to the bluetooth stack to indicate that the device is powered on.
 *
 * @return void
 */
void timeros_init(const uint32_t event_signal_mask) {
	/* Enable the HF Peripheral clock if it isn't already */
	CMU_ClockEnable(cmuClock_HFPER, true);
	/* Enable the I2C Clock */
	CMU_ClockEnable(cmuClock_TIMER0, true);
	/* Cache our event signal mask so that the calling program can take action as appropriate. */
	timer_signal_mask = event_signal_mask;
}

/*
 * @brief Calculates the number of ticks required for the desired delay and produces the optimal prescaler.
 *
 * @param delay The number of seconds to wait. Must be less than HFClockFreq/(1024*65535).
 *
 * @return structure containing the calculated result.
 */
TIMEROS_Delay_TypeDef timeros_calc_ticks(const float delay) {
	uint32_t ticks;
	uint32_t prescaler;
	TIMEROS_Delay_TypeDef result;

	/* Figure out how many unscaled ticks we need */
	ticks = (uint32_t) (delay * ((float) SystemHFClockGet()));

	/* Then so long as we need more than fits */
	prescaler = 0;
	while (ticks > TIMEROS_MAXCNT) {
		/* scale the count down */
		ticks >>= 1;
		/* and increase the prescaler */
		++prescaler;
	}

	/* Check to make sure there wasn't too much time requested */
	DEBUG_ASSERT(prescaler <= TIMEROS_MAXPRESCALE, "Requested time is too long!");

	/* Write the results into the structure and throw it back to the program */
	result.ticks = ticks;
	result.prescaler = prescaler;
	return result;
}

/*
 * @brief Creates an event to call callback after delay has elapsed
 *
 * Once the call returns, it is necessary to call timeros_finish_shot() to clean up
 * the remaining settings.
 *
 * @param delay A delay structure containing the necessary ticks and prescaler settings.
 *
 * @return void
 */
void timeros_do_shot(const TIMEROS_Delay_TypeDef *delay) {
	DEBUG_ASSERT(delay->prescaler <= TIMEROS_MAXPRESCALE, "Prescale Too Large!");
	DEBUG_ASSERT(delay->ticks > 0, "Delay is 0! Must be at least 1.");
	DEBUG_ASSERT(!running, "Timer is already running!");

	/* mark that we've started */
	running = true;

	TIMER_Init_TypeDef init_struct;
	init_struct.enable = true;
	init_struct.mode = _TIMER_CTRL_MODE_UP;
	init_struct.clkSel = timerClkSelHFPerClk;
	init_struct.oneShot = true;
	init_struct.prescale = delay->prescaler;
	init_struct.ati = false;
	init_struct.count2x = false;
	init_struct.debugRun = false;
	init_struct.dmaClrAct = false;
	init_struct.fallAction = timerInputActionNone;
	init_struct.riseAction = timerInputActionNone;
	init_struct.quadModeX4 = false;
	init_struct.sync = false;

	/* Stop the timer */
	TIMER_Enable(TIMER0, false);

	/* Clear any residual values */
	CORE_ATOMIC_SECTION(
		TIMER_IntClear(TIMER0, TIMER_IF_OF);
	)

	/* Block sleeping into EM2, we can only work down to EM1 */
	SLEEP_SleepBlockBegin(sleepEM2);

	/* Set the stop time in the top register */
	TIMER_TopSet(TIMER0, delay->ticks);

	/* Enable the interrupt chain from the device through to the processor */
	TIMER_IntEnable(TIMER0, TIMER_IF_OF);
	NVIC_EnableIRQ(TIMER0_IRQn);

	/* Initialize the timer and start it */
	TIMER_Init(TIMER0, &init_struct);
}

/*
 * @brief Cleans up the leftovers of a timer shot and disables all of the relevant interrupts and devices
 *
 * This allows for the ISR to execute in the minimum possible time.
 *
 * @return structure containing the calculated result.
 */
void timeros_finish_shot() {
	/* turn off the timer */
	TIMER_Enable(TIMER0, false);
	/* And clean up the interrupt chain */
	TIMER_IntClear(TIMER0, TIMER_IF_OF);
	NVIC_DisableIRQ(TIMER0_IRQn);
	/* And mark that we're done. */
	running = false;
}

void TIMER0_IRQHandler() {
	/* if our overflow event occurred */
	if (TIMER_IntGet(TIMER0) & TIMER_IF_OF) {
		/* and tell the synchronous handler that we have a shot event */
		gecko_external_signal(timer_signal_mask);
	}
	/*
	 * Break the interrupt chain. This will prevent causing a fresh interrupt
	 * from happening, and we'll finish cleaning up in timeros_finish_shot.
	 * But even if it isn't called, we'll be okay, we'll just burn a bit more power.
	 */
	TIMER_IntDisable(TIMER0, TIMER_IF_OF);
	/* Reallow sleeping down to EM2. We do this here to allow sleeping deep as early as possible */
	SLEEP_SleepBlockEnd(sleepEM2);
}
