/*
 * @file debug.c
 * @brief Provides an API handling errors and debugging
 *
 * @author John-Michael O'Brien
 * @date Sep 19, 2018
 *
 */

#include <stdio.h>
#include <stdbool.h>
#include <em_core.h>

#include "debug.h"

errorcode_t debug_last_BGAPI_error = bg_err_success;

/*
 * @brief verifies that condition is true. If not, the processor is hung for debugging.
 *
 * @param condition The condition to be verified. False will cause a halt.
 * @param message Describes why the assertion failing should lead to a halt.
 * @param file The name of the file the fault occurred in. Use __FILE__ for greatest convenience.
 * @param line The line of the file where the assert can be found. Use __LINE__ for convenience.
 *
 * @return void
 */
void debug_assert(const bool condition, const char* message, const char* file, const uint32_t line) {
	if (!condition) {
		debug_throw(message, file, line);
	}
}

/*
 * @brief Hangs the processor for debugging.
 *
 * @param message Describes why the assertion failing should lead to a halt.
 * @param file The name of the file the fault occurred in. Use __FILE__ for greatest convenience.
 * @param line The line of the file where the assert can be found. Use __LINE__ for convenience.
 *
 * @return void
 */
void debug_throw(const char* message, const char* file, const uint32_t line) {
	/* Turn off interrupts */
	CORE_ATOMIC_IRQ_DISABLE();
	/* And capture the CPU in a hot hold so we can hit it with the debugger */
	while (1) {

	}
}

/* TODO: Documentation */
void debug_assert_BGAPI_success(const errorcode_t result, const char *message, const char* file, const uint32_t line) {
	/* Track the last error */
	debug_last_BGAPI_error = result;
	/* and make sure we're good. */
	debug_assert(result==bg_err_success, message,file, line);
}

/* TODO: Documentation */
void debug_log(const char* message) {
	printf(message);
	printf("\n");
}
