/*
 * meshconn.c
 *
 *  Created on: Nov 1, 2018
 *      Author: jtc-b
 */

#include "stdio.h"
#include "stdint.h"
// #include "stdbool.h"


/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"
#include <gecko_configuration.h>
#include <mesh_sizes.h>

#include "mesh_utils.h"
#include "debug.h"

#include "meshconn.h"

static void _reset_state();
static void _start_provisioning_beacon();

static void _reset_state() {

}

static void _start_provisioning_beacon() {
	/* Using GATT because I'm provisioning from a non-mesh phone. */
	gecko_cmd_mesh_node_start_unprov_beaconing(MESH_PROV_BEACON_USE_GATT);
}

void meshconn_init() {
	_reset_state();
	debug_log("Initialized.");
}

void meshconn_handle_events(uint32_t evt_id, struct gecko_cmd_packet *evt) {
	uint16_t result;
	switch(evt_id) {
		case gecko_evt_system_boot_id:
			DEBUG_ASSERT_BGAPI_SUCCESS(gecko_cmd_mesh_node_init_oob(
					0,
					MESH_PROV_AUTH_METHOD_STATIC_OOB,
					MESH_PROV_OOB_OUTPUT_ACTIONS_BLINK,
					8,
					MESH_PROV_OOB_INPUT_ACTIONS_NONE,
					0,
					MESH_PROV_OOB_LOCATION_OTHER)
				->result, "Failed to initialize the mesh node feature");
			break;
		case gecko_evt_mesh_node_initialized_id:
			if (evt->data.evt_mesh_node_initialized.provisioned) {
				/* TODO: Start the provisioned init. (Fire "is provisioned event?") */

			} else {
				/* The Node is now initialized, start unprovisioned Beaconing using PB-GATT Bearer. */
				_start_provisioning_beacon();
			}
			break;
		case gecko_evt_mesh_node_provisioning_started_id:
			debug_log("Beginning provisioning...");
			break;
		case gecko_evt_mesh_node_provisioning_failed_id:
			/* If we were trying to be provisioned and failed, then go back to beaconing in hopes of being provisioned */
			break;
		case gecko_evt_mesh_node_display_output_oob_id:
			if (evt->data.evt_mesh_node_display_output_oob.output_action != MESH_PROV_OOB_DISPLY_BLINK) {
				debug_log("Invalid provisioning mode requested.");
				/* We can't do anything with that, so bail and let the user fail the provisioning. */
				return;
			}
			//evt->data.evt_mesh_node_display_output_oob.
			break;
		default:
			break;
	}
}
