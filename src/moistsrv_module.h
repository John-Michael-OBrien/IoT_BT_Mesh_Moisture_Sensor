/*
 * mostsrv_module.h
 *
 *  Created on: Nov 14, 2018
 *      Author: jtc-b
 */

#ifndef SRC_MOISTSRV_MODULE_H_
#define SRC_MOISTSRV_MODULE_H_

#include "stdint.h"
#include "native_gecko.h"

#define MOISTURE_ELEMENT_INDEX (0) /* Why 0? Because their system doesn't currently make an element array constant. >.< */
#define SAVE_DELAY (5000) /* ms */
#define SAVE_TIMER_HANDLE (1)

void moistsrv_init();
void moistsrv_handle_events(uint32_t evt_id, struct gecko_cmd_packet *evt);

#endif /* SRC_MOISTSRV_MODULE_H_ */