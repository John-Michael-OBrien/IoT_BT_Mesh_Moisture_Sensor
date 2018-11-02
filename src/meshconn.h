/*
 * connection_mesh.h
 *
 *  Created on: Nov 1, 2018
 *      Author: jtc-b
 */

#ifndef SRC_MESHCONN_H_
#define SRC_MESHCONN_H_

#define MESH_PUBLIC_KEY 0x5A

void meshconn_init();
void meshconn_handle_events(uint32_t evt_id, struct gecko_cmd_packet *evt);

#endif /* SRC_MESHCONN_H_ */
