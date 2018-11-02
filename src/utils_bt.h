/*
 * @file utils_bt.h
 * @brief Provides core utilities for working with BLE and the Blue Gecko Stack
 *
 * @author John-Michael O'Brien
 * @date Oct 11, 2018
 */

#ifndef SRC_UTILS_BT_H_
#define SRC_UTILS_BT_H_

#include <stdint.h>
#include <stdbool.h>
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"

#define GET_STATIC_ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))

/* BT Setting Constants from specs and manuals */
#define CONNECTION_COUNT_PERIOD (0.00125f) /* Seconds */
#define SLAVE_LATENCY_PERIOD (CONNECTION_MAX_INTERVAL) /* Seconds */
#define SUPERVISORY_TIMEOUT_PERIOD (0.01f) /* Seconds */
#define RADIO_TX_POWER_STEP (0.1f) /* dBm */
#define GATT_TX_POWER_STEP (1.0f) /* dBm */
#define RADIO_MAX_TX_POWER (10.0f) /* dBm */
#define RADIO_MIN_TX_POWER (-30.0f) /* dBm */

/* API Halt Constants */
#define HALTMODE_RESUME (0)
#define HALTMODE_HALT (1)

/* API Increase Security Constants */
#define BT_ALLOW_NO_MITM_PROTECTION (0<<0)
#define BT_REQUIRE_MITM_PROTECTION (1<<0)
#define BT_ALLOW_NO_ENCRYPTION (0<<1)
#define BT_REQUIRE_ENCRYPTION (1<<1)
#define BT_ALLOW_LEGACY_CONNECTION (0<<2)
#define BT_REQUIRE_SECURE_CONNECTION (1<<2)
#define BT_ALLOW_NO_BONDING_CONFIRMATION (0<<3)
#define BT_REQUIRE_BONDING_CONFIRMATION (1<<3)
#define BT_REQUIRE_FULL_SECURITY (BT_REQUIRE_BONDING_CONFIRMATION | BT_REQUIRE_SECURE_CONNECTION | BT_REQUIRE_ENCRYPTION | BT_REQUIRE_MITM_PROTECTION)

#define BT_DISALLOW_BONDING (0)
#define BT_ALLOW_BONDING (1)

#define BT_DECLINE_BONDING (0)
#define BT_ACCEPT_BONDING (1)

#define BT_CANCEL_KEYPAD_BONDING (-1)

#define BT_NO_BONDING (0xFF)

/* Basic Bluetooth Message constants */
#define BT_ADDRESS_LENGTH (6) /* Bytes */
#define ADDRESS_SEGMENT_SIZE (3) /* Bytes */
#define BT_UUID_LENGTH (16) /* bytes */
#define BT_UUID_16BIT_OFFSET (BT_UUID_LENGTH - 4) /* bytes */
#define BT_UUID_16BIT_LENGTH (2) /* bytes */
#define BT_BASE_UUID {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} /* "00000000-0000-1000-8000-00805F9B34FB" */
#define BT_128BIT_INITIALIZER_FROM_16BIT(lower, upper) {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, lower, upper, 0x00, 0x00}
#define BT_CONNECTION_BROADCAST (0xFF)

#define BT_GATT_CLIENT_STATUS_CONFIG_MASK (0x01)
#define BT_GATT_CLIENT_CONFIG_INDICATION_MASK (0x02)

#define BT_DISCOVER_SET_LE_1M_PHY (1)
#define BT_DISCOVER_SET_LE_CODED_PHY (4)
#define BT_DISCOVER_SET_LE_BOTH_PHY (5)

#define BT_DISCOVER_ACTIVE (1)
#define BT_DISCOVER_PASSIVE (0)

#define BT_AD_SCANNABLE (0<<0)
#define BT_AD_NOT_SCANNABLE (1<<0)
#define BT_AD_CONNECTABLE (0<<1)
#define BT_AD_NOT_CONNECTABLE (1<<1)
#define BT_AD_NOT_SCAN_RESPONSE (0<<2)
#define BT_AD_SCAN_RESPONSE (1<<2)

#define BT_AD_ADDR_PUBLIC (0)
#define BT_AD_ADDR_PRIVATE (1)
#define BT_AD_ADDR_ANONYMOUS (0xFF)

#define BT_AD_DATA_COMPLETE (0x00 << 5)
#define BT_AD_DATA_INCOMPLETE (0x10 << 5)

#define BT_AD_NO_BONDING (0xFF)

#define BT_AD_TYPE_INCOMPLETE_16BIT (0x02)
#define BT_AD_TYPE_COMPLETE_16BIT (0x03)
#define BT_AD_TYPE_INCOMPLETE_32BIT (0x04)
#define BT_AD_TYPE_COMPLETE_32BIT (0x05)
#define BT_AD_TYPE_INCOMPLETE_128BIT (0x06)
#define BT_AD_TYPE_COMPLETE_128BIT (0x07)
#define BT_AD_SHORTENED_NAME (0x08)
#define BT_AD_NAME (0x09)

/* Advertisement constantant */
#define ADVERTISEMENT_COUNT_PERIOD (0.000625f) /* Seconds */
#define ADVERTISEMENT_DURATION_UNLIMITED (0)
#define ADVERTISEMENT_EVENTS_UNLIMITED (0)

/* Constants for the bluetooth stack timer API */
#define SOFT_TIMER_STOP (0)
#define SOFT_TIMER_FREE_RUN (0)
#define SOFT_TIMER_ONE_SHOT (1)
#define SOFT_TIMER_STOP_TIMER (0)
#define SOFT_TIMER_FREQUENCY (32768.0f) /* Hz */

/* Macros for converting human numbers into device registers */
#define GET_SOFT_TIMER_COUNTS(time) ((uint32_t)(time * SOFT_TIMER_FREQUENCY))
#define GET_ADVERTISEMENT_COUNTS(time) ((uint32)((time / ADVERTISEMENT_COUNT_PERIOD)))


typedef struct {
	uint8_t data[2];
} uuid_16;

typedef uint32_t IEEE_11073_FLOAT;


extern const uuid_128 bt_base_uuid_const;
extern uint8_t active_connection; /* Who we're talking to or 0xFF if we're not connected. */
extern bool encrypted; /* Whether the connection is currently encrypted. */
extern char prompt_buffer[32]; /* Shared buffer prep space. DO NOT ACCESS IN AN ISR. EXPECT THIS TO BE CLOBBERED ONCE YOU'RE OUT OF SCOPE! */

char* bt_address_to_string(const bd_addr address, const uint8_t count, const uint8_t offset, char *target);
void bt_16bit_to_128bit_uuid(const uint8 *small_uuid, uuid_128 *large_uuid);
int bt_compare_uuids(const uint8_t *uuid1, const uuid_128 *uuid2);
int bt_compare_16bit_uuids(const uint8_t *uuid1, const uuid_16 *uuid2);
float IEEE_11073_to_IEEE_754(IEEE_11073_FLOAT raw_11073);

#endif /* SRC_UTILS_BT_H_ */
