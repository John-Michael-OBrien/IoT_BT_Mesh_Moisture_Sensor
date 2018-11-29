/*
 * @file moistsrv_module.c
 * @brief Moisture Server Module. Takes measurements of the moisture sensor
 *     and transmits them to the mesh network.
 *
 * @author John-Michael O'Brien
 * @date Nov 14, 2018
 */

/* Standard Libraries */
#include "stdio.h"
#include "stdlib.h" /* pulls in malloc and free for us. */
#include "stdint.h"
#include "stdbool.h"

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"
#include <gecko_configuration.h>
#include <mesh_sizes.h>
#include <mesh_generic_model_capi_types.h>
#include <mesh_lib.h>

#include <sleep.h>

#include "meshconn_module.h"
#include "moistsrv_module.h"

#include "lcd_driver.h"
#include "pb_driver_bt.h"
#include "soil_driver.h"

#include "utils_bt.h"
#include "mesh_utils.h"
#include "debug.h"
#include "user_signals_bt.h"

/*
 * Using PACKSTRCT so we don't end up with a bunch of wasted memory.
 * The price we pay is access delays since the struct's members aren't
 * going to be aligned, but we're not doing a lot with this struct, so
 * longer access times are okay.
 */
typedef PACKSTRUCT(struct {
	uint16_t alarm_level;
}) persistent_data;

static persistent_data settings;
static bool disable_deep_sleep = false;
static bool ready = false;
static uint8_t conn_count = 0;


#define ALARM_FLASH_KEY (0x4001)
#define DEFAULT_ALARM_LEVEL (0x7FFF)

static void _toast(char *message);
static void _save_settings();
static void _load_settings();
static void _save_settings_eventually();
static void _set_alarm_level(uint16_t new_level);
static void _publish_level();
static void _update_level(uint16_t level);
void _handle_client_request(uint16_t model_id,
        uint16_t element_index,
        uint16_t client_addr,
        uint16_t server_addr,
        uint16_t appkey_index,
        const struct mesh_generic_request *request,
        uint32_t transition_ms,
        uint16_t delay_ms,
        uint8_t request_flags);
void _handle_server_change(uint16_t model_id,
        uint16_t element_index,
        const struct mesh_generic_state *current,
        const struct mesh_generic_state *target,
        uint32_t remaining_ms);
void _init_and_register_models();
static void _become_lpn();
static void _try_befriending_later();
static void _get_friend();

static void _toast(char *message) {
	LCD_write("", LCD_ROW_ACTION);
	LCD_write(message, LCD_ROW_ACTION);
	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_hardware_set_soft_timer(GET_SOFT_TIMER_COUNTS(TOAST_DURATION), TOAST_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)
			->result, "Failed to save new alarm setting.");
}

// TODO: Documentation
static void _save_settings() {
	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_flash_ps_save(ALARM_FLASH_KEY,2,(uint8_t*) &settings)
			->result, "Failed to save new alarm setting.");
	debug_log("Settings saved.");
}

// TODO: Documentation
static void _save_settings_eventually() {
	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_hardware_set_soft_timer(GET_SOFT_TIMER_COUNTS(SAVE_DELAY), SAVE_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)
			->result, "Failed to save new alarm setting.");
}

// TODO: Documentation
static void _set_alarm_level(uint16_t new_level) {
	settings.alarm_level = new_level;
	sprintf(prompt_buffer, "New: 0x%04X", settings.alarm_level);
	_toast(prompt_buffer);
	_save_settings_eventually();
}

// TODO: Documentation
static void _load_settings() {
	struct gecko_msg_flash_ps_load_rsp_t *result;
	bool loaded = false;

	debug_log("Loading settings starting with defaults...");
	result = gecko_cmd_flash_ps_load(ALARM_FLASH_KEY);

	/* Start with the default */
	settings.alarm_level = DEFAULT_ALARM_LEVEL;

	/* And if we actually got a good result */
	if (result->result == bg_err_success) {
		/* And it looks like the kind of data it's supposed to be */
		if(result->value.len == sizeof(settings)) {
			settings = *((persistent_data*) result->value.data);
			debug_log("Successfully loaded non-default values");
			loaded = true;
		}
	}
	if (!loaded) {
		debug_log("Using defaults and initializing flash...");
		_save_settings();
	}
	debug_log("Finished loading settings.");
}

// TODO: Documentation
static void _publish_level() {
	errorcode_t result;
	result = mesh_lib_generic_server_publish(
			MESH_GENERIC_LEVEL_SERVER_MODEL_ID,
			MOISTURE_ELEMENT_INDEX,
			mesh_generic_state_level);
	debug_log("Published. Result: 0x%04X", result);
}

// TODO: Documentation
static void _update_level(uint16_t level) {
	struct mesh_generic_state outbound_state;
	struct mesh_generic_state next_state;
	errorcode_t result;

	outbound_state.kind = mesh_generic_state_level;
	outbound_state.level.level = level; //level;
	next_state.kind = mesh_generic_state_level;
	next_state.level.level = level; //level;

	result = mesh_lib_generic_server_update(
			MESH_GENERIC_LEVEL_SERVER_MODEL_ID,
			MOISTURE_ELEMENT_INDEX,
			&outbound_state,
			&next_state,
			0);
	debug_log("Updated. Result: 0x%04X", result);
}

// TODO: Documentation
void _handle_client_request(uint16_t model_id,
        uint16_t element_index,
        uint16_t client_addr,
        uint16_t server_addr,
        uint16_t appkey_index,
        const struct mesh_generic_request *request,
        uint32_t transition_ms,
        uint16_t delay_ms,
        uint8_t request_flags) {

	debug_log("Received change request from %04X. Target: %d", client_addr, request->level);
	struct mesh_generic_state old_state;

	if (request_flags & MESH_REQUEST_FLAG_RESPONSE_REQUIRED) {
		old_state.level.level = settings.alarm_level;
		mesh_lib_generic_server_response(
				model_id,
				element_index,
				client_addr,
				appkey_index,
				&old_state,
				&old_state,
				0,
				0);
	} else {
		_set_alarm_level(request->level);
	}
}

// TODO: Documentation
void _handle_server_change(uint16_t model_id,
        uint16_t element_index,
        const struct mesh_generic_state *current,
        const struct mesh_generic_state *target,
        uint32_t remaining_ms) {
	debug_log("Received state change. New target: %d", target->level.level);
}

// TODO: Documentation
void _init_and_register_models() {
	/* Init Mesh Lib now that we're provisioned. */
	debug_log("Starting up meshlib...");
	DEBUG_ASSERT_BGAPI_SUCCESS(mesh_lib_init(malloc, free, 8),
			"Failed to init mesh_lib");

	/* Initialize our model data */
	debug_log("Setting up model data...");

	debug_log("Registering models...");
	DEBUG_ASSERT_BGAPI_SUCCESS(mesh_lib_generic_server_register_handler(
		MESH_GENERIC_LEVEL_SERVER_MODEL_ID,
		MOISTURE_ELEMENT_INDEX, // Why 0? Because their system doesn't currently make an element array constant. >.<
		_handle_client_request,
		_handle_server_change),"Error registering generic level model.");

	_update_level(settings.alarm_level);

	ready = true;
	debug_log("Registered.");
}

// TODO: Documentation
void _become_lpn() {
	debug_log("Becoming friend...");
	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_mesh_lpn_init()
			->result, "Failed to initialize LPN functionality.");

	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_mesh_lpn_configure(LPN_QUEUE_DEPTH, LPN_POLL_TIMEOUT)
			->result, "Failed to set LPN requirements.");

	_get_friend();
}

// TODO: Documentation
void _try_befriending_later() {
	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_hardware_set_soft_timer(GET_SOFT_TIMER_COUNTS(BEFRIEND_RETRY_DELAY), BEFRIEND_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)
			->result, "Failed to start friendship retry timer.");
}

// TODO: Documentation
void _get_friend() {
	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_mesh_lpn_establish_friendship(0)
			->result, "Failed to start looking for a friend.");
}


// TODO: Documentation
void moistsrv_init() {
	soil_init();
	debug_log("ADC Test Reading: %04X\n", soil_get_reading_sync());
	soil_deinit();
}

// TODO: Documentation
void moistsrv_handle_events(uint32_t evt_id, struct gecko_cmd_packet *evt) {
	switch(evt_id) {
		case gecko_evt_system_external_signal_id:
			if (evt->data.evt_system_external_signal.extsignals & CORE_EVT_BOOT) {
	    		_load_settings();
	    		DEBUG_ASSERT_BGAPI_SUCCESS(gecko_cmd_mesh_generic_server_init()
	    				->result,"Failed to init Generic Mesh Server");
			}
			if (evt->data.evt_system_external_signal.extsignals & CORE_EVT_POST_BOOT) {
				if (pb_get_pb1()) {
					disable_deep_sleep = true;
					_toast("Forced Awake");
				}
			}
			if(evt->data.evt_system_external_signal.extsignals & CORE_EVT_NETWORK_READY) {
				/* Initialize the mesh models */
				_init_and_register_models();

				/* If we're allowed to go into deep sleep, switch to low power. */
				if (!disable_deep_sleep) {
					_become_lpn();
				} else {
					debug_log("Skipping LPN mode due to boot button.");
				}
			}
			if(evt->data.evt_system_external_signal.extsignals & PB_EVT_0) {
				debug_log("PB0\n");
				if (meshconn_get_state() == network_ready && ready) {
					_toast("Forced TX");
					_update_level(MOIST_ALARM_FLAG);
					_publish_level();
				}
			}
			break;

	    case gecko_evt_mesh_generic_server_client_request_id:
	        debug_log("gecko_evt_mesh_generic_server_client_request_id");

	        /* forward the event to mesh_lib */
	        mesh_lib_generic_server_event_handler(evt);
	        break;

	    case gecko_evt_mesh_generic_server_state_changed_id:
	        debug_log("gecko_evt_mesh_generic_server_state_changed_id");

	        /* forward the event to mesh_lib */
	    	mesh_lib_generic_server_event_handler(evt);
	    	break;

		case gecko_evt_le_connection_opened_id:
			debug_log("Connection opened. Turning off LPN.");
			++conn_count;
			DEBUG_ASSERT_BGAPI_SUCCESS(
					gecko_cmd_mesh_lpn_init()
					->result, "Failed to initialize LPN functionality.");
			break;

	    case gecko_evt_le_connection_closed_id:
	    	--conn_count;
	    	if (conn_count <= 0) {
	    		if (ready && !disable_deep_sleep) {
	    			debug_log("All connections closed. Turning on LPN.");
	    			_become_lpn();
	    		}
	    	}
			break;

	    case gecko_evt_mesh_lpn_friendship_established_id:
	        debug_log("gecko_evt_mesh_lpn_friendship_established_id");
	        debug_log("Maximum Sleep mode: %d\n",SLEEP_LowestEnergyModeGet());
	        /* Yay! Friends! Do nothing! */
	    	break;

	    case gecko_evt_mesh_lpn_friendship_failed_id:
	        debug_log("gecko_evt_mesh_lpn_friendship_failed_id");

	        /* Try looking for a friend again after a bit. */
	        _try_befriending_later();
	    	break;

	    case gecko_evt_mesh_lpn_friendship_terminated_id:
	        debug_log("gecko_evt_mesh_lpn_friendship_terminated_id");

	        /* Reach out for a new friend since we lost ours. */
	        _get_friend();
	        break;


	    case gecko_evt_hardware_soft_timer_id:
	    	switch (evt->data.evt_hardware_soft_timer.handle) {
	    		case SAVE_TIMER_HANDLE:
	    			/* If we've waited long enough without the settings change being asked for again, commit it to flash. */
	    			_save_settings();
	    			break;
	    		case TOAST_TIMER_HANDLE:
	    			/* After the toast ends, clear the toast. */
	    			LCD_write("", LCD_ROW_ACTION);
	    			break;
	    		case BEFRIEND_TIMER_HANDLE:
	    			/* Retry making friends since we lost our old one or couldn't find one. */
	    			_get_friend();
	    			break;
	    		default:
	    			break;
	    	}
	    	break;

	    default:
			break;
	}
}
