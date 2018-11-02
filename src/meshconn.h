/*
 * connection_mesh.h
 *
 *  Created on: Nov 1, 2018
 *      Author: jtc-b
 */

#ifndef SRC_MESHCONN_H_
#define SRC_MESHCONN_H_

#define MESH_STATIC_KEY {0x12,0x34}


#define BLINK_TIMER_HANDLE (0x00)
#define BLINK_ON_TIME (.3) /* Dit */
#define BLINK_OFF_TIME (1.0 * BLINK_ON_TIME) /* Inter character */
#define BLINK_GAP_TIME (7.0 * BLINK_ON_TIME) /* Inter word */

#define BLINK_ON_COUNTS (GET_SOFT_TIMER_COUNTS(BLINK_ON_TIME))
#define BLINK_OFF_COUNTS (GET_SOFT_TIMER_COUNTS(BLINK_OFF_TIME))
#define BLINK_GAP_COUNTS (GET_SOFT_TIMER_COUNTS(BLINK_GAP_TIME))

void meshconn_init();
void meshconn_handle_events(uint32_t evt_id, struct gecko_cmd_packet *evt);

#endif /* SRC_MESHCONN_H_ */
