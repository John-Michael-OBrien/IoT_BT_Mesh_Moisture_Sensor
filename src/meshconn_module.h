/*
 * connection_mesh.h
 *
 *  Created on: Nov 1, 2018
 *      Author: jtc-b
 */

#ifndef SRC_MESHCONN_MODULE_H_
#define SRC_MESHCONN_MODULE_H_

#include "stdint.h"
#include "native_gecko.h"

#define MESH_STATIC_KEY {0x12,0x34}

#define MESHCONN_TIMER_HANDLE_BASE (0)
#define BLINK_TIMER_HANDLE (MESHCONN_TIMER_HANDLE_BASE + 0)

#define BLINK_ON_TIME (0.3) /* Dit */
#define BLINK_OFF_TIME (1.0 * BLINK_ON_TIME) /* Inter character */
#define BLINK_GAP_TIME (7.0 * BLINK_ON_TIME) /* Inter word */

#define BLINK_ON_COUNTS (GET_SOFT_TIMER_COUNTS(BLINK_ON_TIME))
#define BLINK_OFF_COUNTS (GET_SOFT_TIMER_COUNTS(BLINK_OFF_TIME))
#define BLINK_GAP_COUNTS (GET_SOFT_TIMER_COUNTS(BLINK_GAP_TIME))

#define REBOOT_TIMER_HANDLE (MESHCONN_TIMER_HANDLE_BASE + 1)
#define REBOOT_TIME (2.0) /* s */
#define REBOOT_TIME_COUNTS (GET_SOFT_TIMER_COUNTS(REBOOT_TIME))

typedef enum { init, booted, unprovisioned, provisioning, network_ready, error } meshconn_states;

meshconn_states meshconn_get_state();
void meshconn_init();
void meshconn_handle_events(uint32_t evt_id, struct gecko_cmd_packet *evt);

#endif /* SRC_MESHCONN_MODULE_H_ */
