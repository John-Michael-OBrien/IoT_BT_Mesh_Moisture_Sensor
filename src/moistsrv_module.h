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

#define MOIST_ALARM_FLAG (0x7FFF)

#define MOISTSRV_TIMER_HANDLE_BASE (10)

#define MOISTURE_ELEMENT_INDEX (0) /* Why 0? Because their system doesn't currently make an element array constant. >.< */

#define SAVE_TIMER_HANDLE (MOISTSRV_TIMER_HANDLE_BASE + 1)
#define SAVE_DELAY (10.000) /* s */

#define TOAST_TIMER_HANDLE (MOISTSRV_TIMER_HANDLE_BASE + 2)
#define TOAST_DURATION (3.000) /* s */


void moistsrv_init();
void moistsrv_handle_events(uint32_t evt_id, struct gecko_cmd_packet *evt);

#endif /* SRC_MOISTSRV_MODULE_H_ */
