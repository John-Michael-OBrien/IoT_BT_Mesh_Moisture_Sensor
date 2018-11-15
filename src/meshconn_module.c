/*
 * meshconn.c
 *
 *  Created on: Nov 1, 2018
 *      Author: jtc-b
 */

/* Standard Libraries */
#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"
#include <gecko_configuration.h>
#include <mesh_sizes.h>

#include "meshconn_module.h"

#include "lcd_driver.h"
#include "led_driver.h"
#include "pb_driver_bt.h"

#include "utils_bt.h"
#include "mesh_utils.h"
#include "debug.h"
#include "user_signals_bt.h"


const static uint8_t mesh_static_auth_data[] = MESH_STATIC_KEY;

static uint8_t blink_count;
static uint8_t blinks_remaining;
static bool blink_state;
static bool blinking;

static void _reset_state();
static void _start_provisioning_beacon();
static void _start_blinking(uint8_t count);
static void _stop_blinking();
static void _handle_blinking();


static void _do_factory_reset() {
	debug_log("*** DOING FACTORY RESET ***");
	/* Wipe out the persistent storage (all keys, bindings, and app data. EVERYTHING!) */
	gecko_cmd_flash_ps_erase_all();
	LCD_write("Factory Reset", LCD_ROW_CONNECTION);
}

static void _reset_state() {
	debug_log("_reset_state");
	blink_count = 0;
	blinks_remaining = 0;
	blink_state = false;
	blinking = false;
	LCD_write("Mesh ADDR", LCD_ROW_BTADDR1);
	LCD_write("", LCD_ROW_BTADDR2);
}

static void _start_provisioning_beacon() {
	debug_log("_start_provisioning_beacon");
	/* Using GATT because I'm provisioning from a non-mesh phone. */
	gecko_cmd_mesh_node_start_unprov_beaconing(MESH_PROV_BEACON_USE_GATT);
}

static void _start_blinking(uint8_t count) {
	/* Set our count */
	blink_count = count;

	/* If the blink count is 0 */
	if (count == 0) {
		/* Just turn the LED on continuously. It'll get turned off with _stop_blinking */
		led_on();
		return;
	}

	/* Set up the state machine like we were just at the end of the inter-blink gap. */
	blinks_remaining = count;
	blink_state = false;

	/* Mark that we're supposed to be blinking */
	blinking = true;

	/* And kick things off. */
	_handle_blinking();
}

static void _stop_blinking() {
	/* Mark that we're not supposed to be blinking */
	blinking = false;
	/* Turn off the led */
	led_off();
	/* And prevent any more blink timer events */
	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_hardware_set_soft_timer(SOFT_TIMER_STOP, BLINK_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)->result,
			"Failed to stop blink timer.");
}

static void _handle_blinking() {
	/* Just in case of a race in the event queue, if we're not supposed to be blinking, do nothing. */
	if (!blinking) {
		return;
	}

	if (blink_state) {
		led_off();
		if (blinks_remaining == 0) {
			DEBUG_ASSERT_BGAPI_SUCCESS(
					gecko_cmd_hardware_set_soft_timer(BLINK_GAP_COUNTS, BLINK_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)->result,
					"Failed to start blink timer.");
			blinks_remaining = blink_count;
		} else {
			DEBUG_ASSERT_BGAPI_SUCCESS(
					gecko_cmd_hardware_set_soft_timer(BLINK_OFF_COUNTS, BLINK_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)->result,
					"Failed to start blink timer.");
		}
		blink_state = false;
	} else {
		led_on();
		DEBUG_ASSERT_BGAPI_SUCCESS(
				gecko_cmd_hardware_set_soft_timer(BLINK_ON_COUNTS, BLINK_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)->result,
				"Failed to start blink timer.");
		--blinks_remaining;
		blink_state = true;
	}
}

void meshconn_init() {
	_reset_state();
	debug_log("Initialized.");
}

void meshconn_handle_events(uint32_t evt_id, struct gecko_cmd_packet *evt) {
	switch(evt_id) {
		case gecko_evt_system_boot_id:
			debug_log("evt_system_boot");

			/* If we're booting with the button held down... */
			if (pb_get_pb0()) {
				debug_log("Button pressed. Factory Resetting...");

				_do_factory_reset();

				/* Turn on the LED to indicate that we're reset */
				led_on();

				/* Stall till they let go. */
				while (pb_get_pb0());

				/* And reboot the software */
				gecko_cmd_system_reset(0);
				return;
			}

			DEBUG_ASSERT_BGAPI_SUCCESS(gecko_cmd_mesh_node_init_oob(
					0,
					MESH_PROV_AUTH_METHOD_STATIC_OOB | MESH_PROV_AUTH_METHOD_OUTPUT_OOB,
//					MESH_PROV_OOB_OUTPUT_ACTIONS_BLINK | MESH_PROV_OOB_DISPLY_NUMERIC,
					MESH_PROV_OOB_DISPLY_NUMERIC,
					6,
					MESH_PROV_OOB_INPUT_ACTIONS_NONE,
					0,
					MESH_PROV_OOB_LOCATION_OTHER)
				->result, "Failed to initialize the mesh node feature");
			break;
		case gecko_evt_mesh_node_initialized_id:
			debug_log("evt_mesh_node_initialized");

			if (evt->data.evt_mesh_node_initialized.provisioned) {
				debug_log("Already provisioned.");
				/* If we were already provisioned, tell that to our downstream components. */
				gecko_external_signal(CORE_EVT_NETWORK_READY);

			} else {
				debug_log("We're unprovisioned. Beaconing...");
				/* The Node is now initialized, start unprovisioned Beaconing using PB-GATT Bearer. */
				_start_provisioning_beacon();
			}

			gecko_external_signal(CORE_EVT_BOOT);
			break;
		case gecko_evt_mesh_node_provisioning_started_id:
			debug_log("evt_mesh_node_provisioning_started");
			break;
		case gecko_evt_mesh_node_static_oob_request_id:
			debug_log("evt_mesh_node_static_oob_request");
			gecko_cmd_mesh_node_static_oob_request_rsp(sizeof(mesh_static_auth_data), mesh_static_auth_data);
			break;
		case gecko_evt_mesh_node_display_output_oob_id:
			debug_log("evt_mesh_node_display_output_oob");
			printf("Data size: %u\n", (uint16_t) evt->data.evt_mesh_node_display_output_oob.data.len);
			printf("Data: ");
			for (uint8_t i=0;i<evt->data.evt_mesh_node_display_output_oob.data.len;i++) {
				printf("%02X", evt->data.evt_mesh_node_display_output_oob.data.data[i]);
			}
			printf("\n");

			switch(evt->data.evt_mesh_node_display_output_oob.output_action) {
				case MESH_PROV_OOB_DISPLY_BLINK:
					/* Grab just the LSB of the data
					 * Technically this is bad. The data is 128 bits wide. However, 10 blinks is unreasonable, so handling up to 255 is fine.
					 */
					_start_blinking(evt->data.evt_mesh_node_display_output_oob.data.data[evt->data.evt_mesh_node_display_output_oob.data.len-1]);
					break;
				case MESH_PROV_OOB_DISPLY_NUMERIC:
					sprintf(prompt_buffer,"%06d",(uint16_t) evt->data.evt_mesh_node_display_output_oob.data.data[evt->data.evt_mesh_node_display_output_oob.data.len-2]);
					LCD_write(prompt_buffer,LCD_ROW_PASSKEY);
					break;
				default:
					debug_log("Invalid provisioning mode requested.");
					return;
			}


			break;

		case gecko_evt_mesh_node_provisioning_failed_id:
			debug_log("evt_mesh_node_provisioning_failed");
			printf("Reason: %04X\n",evt->data.evt_mesh_node_provisioning_failed.result);

			_stop_blinking();
			/* If we were trying to be provisioned and failed, then go back to beaconing in hopes of being provisioned */
			_start_provisioning_beacon();
			break;

		case gecko_evt_mesh_node_provisioned_id:
			debug_log("evt_mesh_node_provisioned");
			/* If we've successfully been provisioned, tell that to our downstream components. */
			gecko_external_signal(CORE_EVT_NETWORK_READY);

			_stop_blinking();
			break;

		case gecko_evt_mesh_node_reset_id:
			_do_factory_reset();
			break;

		case gecko_evt_hardware_soft_timer_id:
			switch(evt->data.evt_hardware_soft_timer.handle) {
				case BLINK_TIMER_HANDLE:
					_handle_blinking();
					break;
				default:
					break;
			}
			break;

		case gecko_evt_system_external_signal_id:
			debug_log("evt_system_external_signal");
			if(evt->data.evt_system_external_signal.extsignals & CORE_EVT_BOOT) {
				/* Kick off the second stage boot */
				gecko_external_signal(CORE_EVT_POST_BOOT);
			}
			if(evt->data.evt_system_external_signal.extsignals & CORE_EVT_NETWORK_READY) {
				struct gecko_msg_mesh_node_get_element_address_rsp_t *addr = gecko_cmd_mesh_node_get_element_address(0);
				DEBUG_ASSERT_BGAPI_SUCCESS(addr->result,"Failed to get element address.");
				sprintf(prompt_buffer, "0x%04X", addr->address);
				LCD_write(prompt_buffer, LCD_ROW_BTADDR2);
			}
			break;
		default:
			break;
	}
}
