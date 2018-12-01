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
#include <src/soil_driver_bt.h>

#include "meshconn_module.h"
#include "moistsrv_module.h"

#include "lcd_driver.h"
#include "pb_driver_bt.h"
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
static void _set_alarm_level(uint16_t new_level);
static void _publish_moisture(uint16_t level);
static void _update_level(uint16_t level);
static void _handle_client_request(uint16_t model_id,
        uint16_t element_index,
        uint16_t client_addr,
        uint16_t server_addr,
        uint16_t appkey_index,
        const struct mesh_generic_request *request,
        uint32_t transition_ms,
        uint16_t delay_ms,
        uint8_t request_flags);
static void _handle_server_change(uint16_t model_id,
        uint16_t element_index,
        const struct mesh_generic_state *current,
        const struct mesh_generic_state *target,
        uint32_t remaining_ms);
static void _init_and_register_models();
static void _become_lpn();
static void _get_friend();
static void _do_measurement();
static void _finish_measurement();

/*
 * Why these values? Well, to be honest, the sweet spot for our project will be bewteen lux level 2 and 3.
 * Similarly, the gap between damp and wet sand for this sensor setup seems to land at 0x0D00, with soaked sand
 * being at 0x0D14-ish and damp being at 0x0C50. This will let us demonstrate the alarm thresholds fairly easily.
 * The rest of the numbers are pretty much arbitrary. More engineering would be possible, but realistically this
 * table should be set using a user settings model and a direct measurement mode.
 */
static const uint16_t lux_to_alarm_table[] = {0x0F00,0x0E00,0x0D00,0xC00,0xB00,0xA00,0x900,0x800,0x400,0x300,0x200};
static const uint8_t max_lux = sizeof(lux_to_alarm_table)-1;

/*
 * @brief Briefly displays a message on the LCD.
 *
 * @param message The message to be displayed.
 *
 * @return void
 */
static void _toast(char *message) {
	/* Clear the LCD first. This will cause a flicker for fast toasts so we can tell. */
	LCD_write("", LCD_ROW_ACTION);
	/* Write the message to the LCD */
	LCD_write(message, LCD_ROW_ACTION);
	/* Start the timer to clear the message off the screen. */
	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_hardware_set_soft_timer(GET_SOFT_TIMER_COUNTS(TOAST_DURATION), TOAST_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)
			->result, "Failed to save new alarm setting.");
}

/*
 * @brief Commits the settings to flash.
 *
 * @return void
 */
static void _save_settings() {
	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_flash_ps_save(ALARM_FLASH_KEY,2,(uint8_t*) &settings)
			->result, "Failed to save new alarm setting.");
	debug_log("Settings saved.");
}

/*
 * @brief Sets the new alarm level, updates the associated models, and saves the new settings.
 *
 * @param level The new value our setting should be changed to.
 *
 * @return void
 */
static void _set_alarm_level(uint16_t new_level) {
	/* If it's the same as what we had before, bail */
	if (new_level == settings.alarm_level) {
		return;
	}

	/* Record the new setting */
	settings.alarm_level = new_level;

	/* Show it to the user */
	sprintf(prompt_buffer, "ALM LVL: 0x%04X", settings.alarm_level);
	_toast(prompt_buffer);

	/* Start the timer to save the settings eventually */
	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_hardware_set_soft_timer(GET_SOFT_TIMER_COUNTS(SAVE_DELAY), SAVE_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)
			->result, "Failed to save new alarm setting.");
}

/*
 * @brief Loads settings from flash, and failing that, loads some defaults and saves them.
 *
 * @return void
 */
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
			/* Set our settings */
			settings = *((persistent_data*) result->value.data);
			debug_log("Successfully loaded non-default values");
			loaded = true;
		}
	}

	/* If we weren't able to load the settings */
	if (!loaded) {
		debug_log("Using defaults and initializing flash...");
		/* Push the defaults back to the flash */
		_save_settings();
	}

	debug_log("Finished loading settings. Alarm level loaded is %d.", settings.alarm_level);
}

/*
 * @brief Publishes the moisture level or alarm to the network.
 *
 * @param level The moisture level to be published to the network.
 *
 * @return void
 */
static void _publish_moisture(uint16_t level) {
	errorcode_t result;

	/* Update the model */
	_update_level(level);

	/* And send the value to the mesh */
	result=mesh_lib_generic_server_publish(
				MESH_GENERIC_LEVEL_SERVER_MODEL_ID,
				MOISTURE_ELEMENT_INDEX,
				mesh_generic_state_level);

	/* If something goes wrong, say something. */
	if (result != bg_err_success) {
		sprintf(prompt_buffer,"P-ERR: 0x%04X", result);
		_toast(prompt_buffer);
	}

	debug_log("Published. Result: 0x%04X", result);
}

/*
 * @brief Updates the BGAPI copy of our data
 *
 * @param level The new moisture value our model should be updated to.
 *
 * @return void
 */
static void _update_level(uint16_t level) {
	struct mesh_generic_state outbound_state;
	struct mesh_generic_state next_state;
	errorcode_t result;

	/* Build up two copies of the state structure */
	outbound_state.kind = mesh_generic_state_level;
	outbound_state.level.level = level; //level;
	next_state.kind = mesh_generic_state_level;
	next_state.level.level = level; //level;

	/* And hand it to mesh_lib so it can update the BGAPI */
	result = mesh_lib_generic_server_update(
			MESH_GENERIC_LEVEL_SERVER_MODEL_ID,
			MOISTURE_ELEMENT_INDEX,
			&outbound_state,
			&next_state,
			0);

	/* If something goes wrong, say something. */
	if (result != bg_err_success) {
		sprintf(prompt_buffer,"U-ERR: 0x%04X", result);
		_toast(prompt_buffer);
	}

	debug_log("Updated. Result: 0x%04X", result);
}

/*
 * @brief Callback for server status changes to the generic level model
 *
 * Normally we'd update our variables here, but because of the multiplexing of the
 * model, this function is too generic for us to do that since it gets called with
 * both internal and external changes. Since we do different things for Get/Set and
 * internal updates (which are often temporary) we do nothing here.
 *
 * @param element_index The index of the element which the request is being sent to.
 * @param client_addr The address of the client that is making the request.
 * @param server_addr The address that the request was sent to.
 * @param appkey_index The index of the key that is associated with this request.
 * @param appkey_index The index of the key that is associated with this request.
 * @param request The actual request data itself.
 * @param transition_ms The number of milliseconds we should take to make the change.
 * @param delay_ms The number of milliseconds we should wait before starting the change.
 * @param request_flags The flags associated with the request (direct request/provide a response.)
 *
 * @return void
 */
static void _handle_client_request(uint16_t model_id,
        uint16_t element_index,
        uint16_t client_addr,
        uint16_t server_addr,
        uint16_t appkey_index,
        const struct mesh_generic_request *request,
        uint32_t transition_ms,
        uint16_t delay_ms,
        uint8_t request_flags) {

	uint16_t lux_level;
	struct mesh_generic_state old_state;
	errorcode_t result;

	debug_log("Incoming request. Kind: %d.", request->kind);
	/* If it's not a generic level request, bail */
	if (request->kind != mesh_generic_request_level) {
		return;
	}

	/* Cache the value, so we can change it safely */
	lux_level = request->level;

	/* If it's an alarm, bail; we don't work with those. */
	if (lux_level == MOIST_ALARM_FLAG) {
		return;
	}

	debug_log("Received change request from %04X. Target: %d", client_addr, request->level);

	/* If it's off the end of our table */
	if (lux_level > max_lux) {
		/* Saturate to the end of the table */
		lux_level = max_lux;
	}

	/* Get the new level from the table and set that as the alarm level */
	_set_alarm_level(lux_to_alarm_table[lux_level]);

	/* If we are supposed to respond  */
	if (request_flags & MESH_REQUEST_FLAG_RESPONSE_REQUIRED) {
		/* Set the level to our alarm level */
		old_state.level.level = settings.alarm_level;
		/* And send that to our client. */
		result = mesh_lib_generic_server_response(
				model_id,
				element_index,
				client_addr,
				appkey_index,
				&old_state,
				&old_state,
				0,
				0);
		/* If something goes wrong, say something. */
		if (result != bg_err_success) {
			sprintf(prompt_buffer,"R-ERR: 0x%04X", result);
			_toast(prompt_buffer);
		}

		debug_log("Responded. Result: 0x%04X", result);
	}
}

/*
 * @brief Callback for server status changes to the generic level model
 *
 * Normally we'd update our variables here, but because of the multiplexing of the
 * model, this function is too generic for us to do that since it gets called with
 * both internal and external changes. Since we do different things for Get/Set and
 * internal updates (which are often temporary) we do nothing here.
 *
 * @param model_id The ID of the model that the callback is handling.
 * @param current The last model state that the BGAPI had before the change.
 * @param target The last model state that that we are supposed to change to.
 * @param remaining_ms The number of milliseconds until we're supposed to finish the change.
 *
 * @return void
 */
static void _handle_server_change(uint16_t model_id,
        uint16_t element_index,
        const struct mesh_generic_state *current,
        const struct mesh_generic_state *target,
        uint32_t remaining_ms) {
	debug_log("Moisture model changed. Level: %d", target->level.level);
}

/*
 * @brief Initializes mesh_lib, registers our model event handlers, and initializes our internal state.
 *
 * @return void
 */
static void _init_and_register_models() {
	/* Init mesh_lib now that we're provisioned. */
	debug_log("Starting up meshlib...");
	DEBUG_ASSERT_BGAPI_SUCCESS(mesh_lib_init(malloc, free, 8),
			"Failed to init mesh_lib");

	/* Register our model handler. */
	debug_log("Registering models...");
	DEBUG_ASSERT_BGAPI_SUCCESS(mesh_lib_generic_server_register_handler(
		MESH_GENERIC_LEVEL_SERVER_MODEL_ID,
		MOISTURE_ELEMENT_INDEX, // Why 0? Because their system doesn't currently make an element array constant. >.<
		_handle_client_request,
		_handle_server_change),"Error registering generic level model.");

	/* Mark that we're fully configured and it's safe to make calls against mesh_lib */
	ready = true;
	debug_log("Registered.");
}

/*
 * @brief Initializes the LPN subsystem, configures it, and attempts to establish a friendship.
 *
 * @return void
 */
static void _become_lpn() {
	debug_log("Becoming friend...");
	LCD_write("Not Friended", LCD_ROW_CONNECTION);
	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_mesh_lpn_init()
			->result, "Failed to initialize LPN functionality.");

	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_mesh_lpn_configure(LPN_QUEUE_DEPTH, LPN_POLL_TIMEOUT)
			->result, "Failed to set LPN requirements.");

	_get_friend();
}

/*
 * @brief Attempts to establish a connection to a friend.
 *
 * Mostly just a wrapper on gecko_cmd_mesh_lpn_establish_friendship to allow for unification
 * of code paths as this gets called a few different ways.
 *
 * @return void
 */
static void _get_friend() {
	DEBUG_ASSERT_BGAPI_SUCCESS(
			gecko_cmd_mesh_lpn_establish_friendship(0)
			->result, "Failed to start looking for a friend.");
	LCD_write("Befriending", LCD_ROW_CONNECTION);
}

/*
 * @brief Starts powering on the ADC. Will raise an event when the power on delays are finished.
 *
 * @return void
 */
static void _do_measurement() {
	/* Turn on the ADC and sensor */
	soil_start_reading_async();
}

/*
 * @brief Finishes the measurement and reports the results.
 *
 * Should be called when ADC_WAIT_FINISHED occurs.
 *
 * @return void
 */
static void _finish_measurement() {
	uint16_t measurement;

	/* Make the measurement */
	measurement = soil_finish_reading_async();
	debug_log("ADC Reading: %04X against %04X threshold", measurement, settings.alarm_level);

	/* If we're over the limit... */
	if (measurement > settings.alarm_level) {
		debug_log("Sending Alarm.");

		/* Publish the moisture alarm to the group */
		_publish_moisture(MOIST_ALARM_FLAG);

		/* Make the wet (alarmed) prompt */
		sprintf(prompt_buffer,"Wet: 0x%04X",measurement);
	} else {
		/* Make the dry (unalarmed) prompt */
		sprintf(prompt_buffer,"Dry: 0x%04X",measurement);
	}

	/* Write the prompt to the screen */
	LCD_write(prompt_buffer,LCD_ROW_TEMPVALUE);

	/* Send the measurement to the group */
	_publish_moisture(measurement);
}

/*
 * @brief Initializes the Moisture Server module. This includes taking the initial
 * moisture measurement.
 *
 * @return void
 */
void moistsrv_init() {
	/* If PB1 is held down at boot, disable deep sleeping (EM2 and LPN operation) */
	if (pb_get_pb1()) {
		disable_deep_sleep = true;
		debug_log("Will not enter LPN mode due to boot button.");
	}
	/* Prep the sensor library */
	soil_init(ADC_WAIT_FINISHED);
}

/*
 * @brief Responds to events generated by the BGAPI message queue
 * that are related to the moisture server module.
 *
 * @param evt_id The ID of the event.
 * @param evt A pointer to the structure holding the event data.
 *
 * @return void
 */
void moistsrv_handle_events(uint32_t evt_id, struct gecko_cmd_packet *evt) {
	switch(evt_id) {
		case gecko_evt_system_external_signal_id:
			if (evt->data.evt_system_external_signal.extsignals & CORE_EVT_BOOT) {
	    		_load_settings();
	    		DEBUG_ASSERT_BGAPI_SUCCESS(gecko_cmd_mesh_generic_server_init()
	    				->result,"Failed to init Generic Mesh Server");
			}
			if (evt->data.evt_system_external_signal.extsignals & CORE_EVT_POST_BOOT) {
				/* If we've had our sleep mode disabled, report that to the user. */
				if (disable_deep_sleep) {
					_toast("Forced Awake");
				}
			}
			if(evt->data.evt_system_external_signal.extsignals & CORE_EVT_NETWORK_READY) {
				/* Initialize the mesh models */
				_init_and_register_models();

				/* Start taking measurements */
		    	DEBUG_ASSERT_BGAPI_SUCCESS(
		    			gecko_cmd_hardware_set_soft_timer(GET_SOFT_TIMER_COUNTS(MEASUREMENT_TIME), MEASUREMENT_TIMER_HANDLE, SOFT_TIMER_FREE_RUN)
		    			->result, "Failed to start measurement timer.");

				/* If we're allowed to go into deep sleep, switch to low power. */
				if (!disable_deep_sleep) {
					_become_lpn();
				} else {
					LCD_write("Fully Awake", LCD_ROW_CONNECTION);
				}
			}
			if(evt->data.evt_system_external_signal.extsignals & PB_EVT_0) {
				debug_log("PB0\n");
				if (meshconn_get_state() == network_ready && ready) {
					_toast("Forced TX");
					_publish_moisture(MOIST_ALARM_FLAG);
				}
			}
			if(evt->data.evt_system_external_signal.extsignals & ADC_WAIT_FINISHED) {
				_finish_measurement();
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
					gecko_cmd_mesh_lpn_deinit()
					->result, "Failed to initialize LPN functionality.");
			LCD_write("Awake for Connection", LCD_ROW_CONNECTION);
			_toast("External Connect");
			break;

	    case gecko_evt_le_connection_closed_id:
	    	--conn_count;
			_toast("External Disconnect");
	    	if (conn_count <= 0) {
	    		if (ready && !disable_deep_sleep) {
	    			debug_log("All connections closed. Turning on LPN.");
	    			_become_lpn();
	    		} else if (ready && disable_deep_sleep) {
	    			LCD_write("Fully Awake", LCD_ROW_CONNECTION);
	    		}
	    	}
			break;

	    case gecko_evt_mesh_lpn_friendship_established_id:
	        debug_log("gecko_evt_mesh_lpn_friendship_established_id");
	        debug_log("Maximum Sleep mode: %d\n",SLEEP_LowestEnergyModeGet());
	    	_toast("Friend Found");
			LCD_write("Friended", LCD_ROW_CONNECTION);
	        /* Yay! Friends! Do nothing! */
	    	break;

	    case gecko_evt_mesh_lpn_friendship_failed_id:
	        debug_log("gecko_evt_mesh_lpn_friendship_failed_id");

	        _toast("Befriend Failed");
	    	LCD_write("Friend Wait", LCD_ROW_CONNECTION);

	    	/* Try looking for a friend again after a bit. */
	    	DEBUG_ASSERT_BGAPI_SUCCESS(
	    			gecko_cmd_hardware_set_soft_timer(GET_SOFT_TIMER_COUNTS(BEFRIEND_RETRY_DELAY), BEFRIEND_TIMER_HANDLE, SOFT_TIMER_ONE_SHOT)
	    			->result, "Failed to start friendship retry timer.");
	    	break;

	    case gecko_evt_mesh_lpn_friendship_terminated_id:
	        debug_log("gecko_evt_mesh_lpn_friendship_terminated_id");

	    	LCD_write("Not Friended", LCD_ROW_CONNECTION);
	    	_toast("Lost Friend");

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
	    		case MEASUREMENT_TIMER_HANDLE:
	    			/* Make and (if necessary) report the measurement. */
	    			_do_measurement();
	    			break;
	    		default:
	    			break;
	    	}
	    	break;

	    default:
			break;
	}
}
