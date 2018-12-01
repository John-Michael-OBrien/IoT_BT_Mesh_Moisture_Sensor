/*
 * @file moistsrv_module.h
 * @brief Moisture Server Module. Takes measurements of the moisture sensor
 *     and transmits them to the mesh network.
 *
 * @author John-Michael O'Brien
 * @date Nov 14, 2018
 */

#ifndef SRC_MOISTSRV_MODULE_H_
#define SRC_MOISTSRV_MODULE_H_

#include "stdint.h"
#include "native_gecko.h"

/* Primary performance tuning parameters. */
#define MEASUREMENT_TIME (5.000) /* s */
#define LPN_POLL_TIMEOUT (30000) /* ms */
#define BEFRIEND_RETRY_DELAY (19.000) /* s */
#define SAVE_DELAY (10.000) /* s */

/* How long to keep temporary notices (toasts) on the screen */
#define TOAST_DURATION (3.000) /* s */

#define MOIST_ALARM_FLAG (0x7FFF)

#define LPN_QUEUE_DEPTH (4) /* We use 4 because the defaults allow for up to 5 */
#define MOISTURE_ELEMENT_INDEX (0) /* Why 0? Because their system doesn't currently make an element array constant. >.< */

#define MOISTSRV_TIMER_HANDLE_BASE (10)
#define SAVE_TIMER_HANDLE (MOISTSRV_TIMER_HANDLE_BASE + 0)
#define TOAST_TIMER_HANDLE (MOISTSRV_TIMER_HANDLE_BASE + 1)
#define BEFRIEND_TIMER_HANDLE (MOISTSRV_TIMER_HANDLE_BASE + 2)
#define MEASUREMENT_TIMER_HANDLE (MOISTSRV_TIMER_HANDLE_BASE + 3)

void moistsrv_init();
void moistsrv_handle_events(uint32_t evt_id, struct gecko_cmd_packet *evt);

#endif /* SRC_MOISTSRV_MODULE_H_ */
