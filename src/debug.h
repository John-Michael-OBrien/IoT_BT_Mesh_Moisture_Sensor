/*
 * @file debug.h
 * @brief Provides an API handling errors and debugging
 *
 * @author John-Michael O'Brien
 * @date Sep 19, 2018
 *
 */

#ifndef SRC_DEBUG_H_
#define SRC_DEBUG_H_

#include "stdint.h"
#include "stdbool.h"
#include "bg_errorcodes.h"

/* Define this to enable verbose debug output */
#define DEBUG_VERBOSE

/* Macros for automatically including line numbers and file names */
#define DEBUG_ASSERT(condition,message) debug_assert(condition, message, __FILE__, __LINE__)
#define DEBUG_ASSERT_BGAPI_SUCCESS(result,message) debug_assert_BGAPI_success(result, message, __FILE__, __LINE__)
#define DEBUG_THROW(message) debug_throw(message, __FILE__, __LINE__)

void debug_assert(const bool condition, const char* message, const char* file, const uint32_t line);
void debug_throw(const char* message, const char* file, const uint32_t line);
void debug_assert_BGAPI_success(const errorcode_t result, const char *message, const char* file, const uint32_t line);
void debug_log(const char* format, ...);

#endif /* SRC_DEBUG_H_ */
