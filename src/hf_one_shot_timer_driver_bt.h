/*
 * @file hf_one_shot_timer_driver.h
 * @brief Provides an API timing out brief intervals via callbacks
 *
 * Utilizes the TIMER0 resource.
 *
 * @author John-Michael O'Brien
 * @date Sep 20, 2018
 */

#ifndef SRC_HF_ONE_SHOT_TIMER_DRIVER_BT_H_
#define SRC_HF_ONE_SHOT_TIMER_DRIVER_BT_H_

#define TIMEROS_MAXCNT (0xFFFF)
#define TIMEROS_MAXPRESCALE (10) /* for error checking from the reference */

#include <stddef.h>
#include <stdint.h>

/* Holds all we need for setting up the timer for a given delay */
typedef struct {
	uint16_t ticks;
	uint8_t prescaler;
} TIMEROS_Delay_TypeDef;

void timeros_init(const uint32_t event_signal_mask);
TIMEROS_Delay_TypeDef timeros_calc_ticks(const float delay);
void timeros_do_shot(const TIMEROS_Delay_TypeDef *delay);
void timeros_finish_shot();

#endif /* SRC_HF_ONE_SHOT_TIMER_DRIVER_BT_H_ */
