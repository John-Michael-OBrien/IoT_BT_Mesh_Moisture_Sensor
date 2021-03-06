/*
 * @file meshconn_module.c
 * @brief Module that provides basic connection and provisioning services
 *
 * @author John-Michael O'Brien
 * @date Nov 1, 2018
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

#define OUR_ADDRESS_LENGTH (6)
#define OUR_ADDRESS_OFFSET (BT_ADDRESS_LENGTH - OUR_ADDRESS_LENGTH)
#define MESHCONN_SECURE_DIGITS (4)


const static uint8_t mesh_static_auth_data[] = MESH_STATIC_KEY;

static uint8_t blink_count;
static uint8_t blinks_remaining;
static bool blink_state;
static bool blinking;
static meshconn_states state = init;
static uint8_t conn_handle;

static void _reset_state();
static void _start_provisioning_beacon();
static void _start_blinking(uint8_t count);
static void _stop_blinking();
static void _handle_blinking();
static void _activate_network();

/*
 * @brief Clears the settings flash including the provisioning state.
 * Also resets the state to unprovisioned.
 *
 * @return void
 */
static void _do_factory_reset() {
	debug_log("*** DOING FACTORY RESET ***");
	/* Wipe out the persistent storage (all keys, bindings, and app data. EVERYTHING!) */
	gecko_cmd_flash_ps_erase_all();
	LCD_write("Factory Reset", LCD_ROW_CONNECTION);
	state = unprovisioned;
}

/*
 * @brief Resets all internal state for the connection system.
 *
 * @return void
 */
static void _reset_state() {
	debug_log("_reset_state");
	blink_count = 0;
	blinks_remaining = 0;
	blink_state = false;
	blinking = false;
	state = booted;
	LCD_write("", LCD_ROW_BTADDR1);
	LCD_write("Mesh ADDR", LCD_ROW_BTADDR2);
	LCD_write("", LCD_ROW_CLIENTADDR);
	LCD_write("", LCD_ROW_PASSKEY);
	LCD_write("Booting...", LCD_ROW_CONNECTION);
}

/*
 * @brief Begins issuing the provisioning beacon.
 *
 * @return void
 */
static void _start_provisioning_beacon() {
	debug_log("_start_provisioning_beacon");
	/* Using GATT because I'm provisioning from a non-mesh phone. */
	gecko_cmd_mesh_node_start_unprov_beaconing(MESH_PROV_BEACON_USE_GATT);
	state = unprovisioned;
	LCD_write("Beaconing...", LCD_ROW_CONNECTION);
}

/*
 * @brief Starts blinking the indicator LED with a pause between sets.
 *
 * @param count The number of blinks between pauses.
 *
 * @return void
 */
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

/*
 * @brief Stops any blinking on the indicator.
 *
 * @return void
 */
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

/*
 * @brief Turns the light on if it was off, off if it was on, and pauses. A long pause will be used
 * after the chosen number of blinks.
 *
 * @return void
 */
static void _handle_blinking() {
	/* Just in case of a race in the event queue, if we're not supposed to be blinking, do nothing. */
	if (!blinking) {
		return;
	}

	/* If we're on */
	if (blink_state) {
		/* Turn off */
		led_off();
		/* If we've blinked enough */
		if (blinks_remaining == 0) {
			/* Put in the long gap pause */
			DEBUG_ASSERT_BGAPI_SUCCESS(
					gecko_cmd_hardware_set_soft_timer(BLINK_GAP_COUNTS, BLINK_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)->result,
					"Failed to start blink timer.");
			/* And restart the count */
			blinks_remaining = blink_count;
		} else {
			/* Otherwise, but in the short pause */
			DEBUG_ASSERT_BGAPI_SUCCESS(
					gecko_cmd_hardware_set_soft_timer(BLINK_OFF_COUNTS, BLINK_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)->result,
					"Failed to start blink timer.");
		}
		/* Finally, mark that we're off right now. */
		blink_state = false;
	} else {
		/* If we were off, turn on */
		led_on();
		/* Start the short pause */
		DEBUG_ASSERT_BGAPI_SUCCESS(
				gecko_cmd_hardware_set_soft_timer(BLINK_ON_COUNTS, BLINK_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)->result,
				"Failed to start blink timer.");
		/* Reduce the number of blinks outstanding */
		--blinks_remaining;
		/* and finally mark that we're on now. */
		blink_state = true;
	}
}

/*
 * @brief Signals to our event consumers and the user that we're provisioned and ready.
 *
 * @return void
 */
static void _activate_network() {
	/* If we were already provisioned, tell that to our downstream components. */
	state = network_ready;
	gecko_external_signal(CORE_EVT_NETWORK_READY);
	LCD_write("Ready", LCD_ROW_CONNECTION);
}

/*
 * @brief Exposes our internal state to any external consumers. Read only.
 *
 * @return meshconn_states value that is the connection's current state.
 */
meshconn_states meshconn_get_state() {
	return state;
}

/*
 * @brief Initializes the meshconn module.
 *
 * This is mostly a dummy function to maintain module consistency.
 *
 * @return void
 */
void meshconn_init() {
	debug_log("Initialized.");
}

/*
 * @brief Responds to events generated by the BGAPI message queue
 * that are related to the meshconn module.

 * @param evt_id The ID of the event.
 * @param evt A pointer to the structure holding the event data.
 *
 * @return void
 */
void meshconn_handle_events(uint32_t evt_id, struct gecko_cmd_packet *evt) {
	uint8_t *oob_data;
	uint8_t oob_offset;
	uint16_t oob_value;
	switch(evt_id) {
		case gecko_evt_system_boot_id:
			debug_log("evt_system_boot");
			_reset_state();
			pb_start();


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

			/* Put our address on the screen. */
			/* This makes provisioning much easier. */
			LCD_write(
				bt_address_to_string(
					gecko_cmd_system_get_bt_address()->address,
					OUR_ADDRESS_LENGTH,
					OUR_ADDRESS_OFFSET,
					prompt_buffer),
					LCD_ROW_BTADDR1);

			DEBUG_ASSERT_BGAPI_SUCCESS(gecko_cmd_mesh_node_init_oob(
					0,
					MESH_PROV_AUTH_METHOD_STATIC_OOB | MESH_PROV_AUTH_METHOD_OUTPUT_OOB,
//					MESH_PROV_OOB_OUTPUT_ACTIONS_BLINK | MESH_PROV_OOB_OUTPUT_ACTIONS_NUMERIC,
					MESH_PROV_OOB_OUTPUT_ACTIONS_NUMERIC,
					MESHCONN_SECURE_DIGITS,
					MESH_PROV_OOB_INPUT_ACTIONS_NONE,
					0,
					MESH_PROV_OOB_LOCATION_OTHER)
				->result, "Failed to initialize the mesh node feature");
			break;

		case gecko_evt_mesh_node_initialized_id:
			debug_log("evt_mesh_node_initialized");

			if (evt->data.evt_mesh_node_initialized.provisioned) {
				debug_log("Already provisioned.");

				_activate_network();
			} else {
				debug_log("We're unprovisioned. Beaconing...");
				/* The Node is now initialized, start unprovisioned Beaconing using PB-GATT Bearer. */
				_start_provisioning_beacon();
			}

			gecko_external_signal(CORE_EVT_BOOT);
			break;

		case gecko_evt_mesh_node_provisioning_started_id:
			debug_log("evt_mesh_node_provisioning_started");
			state = provisioning;
			LCD_write("Provisioning...", LCD_ROW_CONNECTION);
			LCD_write("", LCD_ROW_PASSKEY);
			break;

		case gecko_evt_mesh_node_static_oob_request_id:
			debug_log("evt_mesh_node_static_oob_request");
			gecko_cmd_mesh_node_static_oob_request_rsp(sizeof(mesh_static_auth_data), mesh_static_auth_data);
			break;

		case gecko_evt_mesh_node_display_output_oob_id:
			debug_log("evt_mesh_node_display_output_oob");

			switch(evt->data.evt_mesh_node_display_output_oob.output_action) {
				case MESH_PROV_OOB_DISPLY_BLINK:
					/* Grab just the LSB of the data
					 * Technically this is bad. The data is 128 bits wide. However, 10 blinks is unreasonable, so handling up to 255 is fine.
					 */
					_start_blinking(evt->data.evt_mesh_node_display_output_oob.data.data[evt->data.evt_mesh_node_display_output_oob.data.len-1]);
					break;
				case MESH_PROV_OOB_DISPLY_NUMERIC:
					oob_data = evt->data.evt_mesh_node_display_output_oob.data.data;
					oob_offset = evt->data.evt_mesh_node_display_output_oob.data.len - 2;
					oob_value = (((uint16_t) oob_data[oob_offset]) << 8) | ((uint16_t) oob_data[oob_offset+1]);
					sprintf(prompt_buffer,"%04d",oob_value);
					LCD_write(prompt_buffer,LCD_ROW_PASSKEY);
					break;
				default:
					sprintf(prompt_buffer, "Invalid mode: %d", (uint16_t) evt->data.evt_mesh_node_display_output_oob.output_action);
					debug_log(prompt_buffer);
					return;
			}

			break;

		case gecko_evt_mesh_node_provisioning_failed_id:
			debug_log("evt_mesh_node_provisioning_failed");
			printf("Reason: %04X\n",evt->data.evt_mesh_node_provisioning_failed.result);
			LCD_write("", LCD_ROW_PASSKEY);

			_stop_blinking();
			/* If we were trying to be provisioned and failed, then go back to beaconing in hopes of being provisioned */
			_start_provisioning_beacon();
			break;

		case gecko_evt_mesh_node_provisioned_id:
			debug_log("evt_mesh_node_provisioned");
			LCD_write("", LCD_ROW_PASSKEY);

			_activate_network();
			_stop_blinking();
			break;

		case gecko_evt_mesh_node_reset_id:
			/* Clear our settings */
			_do_factory_reset();
			LCD_write("Rebooting...",LCD_ROW_CONNECTION);

			/* Close any open connections */
			if (conn_handle != 0xFF) {
				gecko_cmd_le_connection_close(conn_handle);
			}

			DEBUG_ASSERT_BGAPI_SUCCESS(
					gecko_cmd_hardware_set_soft_timer(REBOOT_TIME_COUNTS, REBOOT_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)->result,
					"Failed to start reboot timer.");
			break;

		case gecko_evt_hardware_soft_timer_id:
			switch(evt->data.evt_hardware_soft_timer.handle) {
				case BLINK_TIMER_HANDLE:
					_handle_blinking();
					break;
				case REBOOT_TIMER_HANDLE:
					/* And reboot the software */
					gecko_cmd_system_reset(0);
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
				LCD_write(prompt_buffer, LCD_ROW_CLIENTADDR);
			}
			break;

		case gecko_evt_le_connection_opened_id:
			printf("gecko_evt_le_connection_opened_id\n");
			conn_handle = evt->data.evt_le_connection_opened.connection;
			break;

	    case gecko_evt_le_connection_closed_id:
			printf("gecko_evt_le_connection_closed_id\n");
			conn_handle = 0xFF;
			break;

	    default:
			break;
	}
}
