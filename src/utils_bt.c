/*
 * @file utils_bt.c
 * @brief Provides core utilities for working with BLE and the Blue Gecko Stack
 *
 * @author John-Michael O'Brien
 * @date Oct 11, 2018
 */

#ifndef SRC_UTILS_C_
#define SRC_UTILS_C_

#include <stdint.h>
#include "utils_bt.h"
#include "math.h"


char prompt_buffer[32];

static const char hex_table[] = "0123456789ABCDEF";
const uuid_128 bt_base_uuid_const = {.data = BT_BASE_UUID};

/*
 * @brief Converts a bluetooth address to a string with segmentation
 *
 * @param address The addres structure to be converted to a string
 * @param count The number of address segments to render (2 would produce AA:BB\0)
 * @param offset The byte on which to start rendering the address from
 * @param target The buffer to render the address to.
 *
 * @return A pointer to target for convenience.
 */
char* bt_address_to_string(const bd_addr address, const uint8_t count, const uint8_t offset, char *target) {
	uint8_t last_byte = count-1;
	/* For each byte in a bluetooth address */
	for (int i=0;i<count;i++) {
		/* Calculate the exact position of our byte */
		uint8_t strpos=i*ADDRESS_SEGMENT_SIZE;
		/*
		 * Decode the upper nibble into a hex value. i+offset is subtracted because
		 * the data is encoded in the reverse order of the human readable format.
	     */
		target[strpos]=hex_table[address.addr[BT_ADDRESS_LENGTH-1-(i+offset)] >> 4];
		/* Decode the lower nibble into a hex value */
		target[strpos+1]=hex_table[address.addr[BT_ADDRESS_LENGTH-1-(i+offset)] & 0x0F];

		/* if this isn't the last byte */
		if (i<last_byte) {
			/* Separate bytes with a colon */
			target[strpos+2] = ':';
		} else {
			/* Otherwise, end the string with a null terminator */
			target[strpos+2] = '\0';
		}
	}

	/* Convenience return, reiterates the input pointer */
	return target;
}

/*
 * @brief Pads a 16 bit UUID into a 128 bit UUID
 *
 * @param small_uuid A pointer to the raw bytes of the 16 bit UUID
 * @param large_uuid A pointer to a 128 bit UUID structure
 *
 * @return A pointer to target for convenience.
 */
void bt_16bit_to_128bit_uuid(const uint8 *small_uuid, uuid_128 *large_uuid) {
	// Copy the base address in.
	memcpy(large_uuid->data, bt_base_uuid_const.data, BT_UUID_LENGTH);
	// Copy the 16 bit portion into the appropriate space.
	memcpy((large_uuid->data)+BT_UUID_16BIT_OFFSET, small_uuid, BT_UUID_16BIT_LENGTH );
}

/*
 * @brief Compares two 128 bit UUIDs in a bytewise manner
 *
 * @param uuid1 The uuid to be used for comparison
 * @param uuid2 The uuid to be compared to
 *
 * @return Returns a 0 if they match. anything else indicates a mismatch.
 */
int bt_compare_uuids(const uint8_t *uuid1, const uuid_128 *uuid2) {
	return memcmp(uuid1, uuid2->data, BT_UUID_LENGTH);
}

/*
 * @brief Compares two 16 bit UUIDs in a bytewise manner
 *
 * @param uuid1 The uuid to be used for comparison
 * @param uuid2 The uuid to be compared to
 *
 * @return Returns a 0 if they match. anything else indicates a mismatch.
 */
int bt_compare_16bit_uuids(const uint8_t *uuid1, const uuid_16 *uuid2) {
	return memcmp(uuid1, uuid2->data, BT_UUID_16BIT_LENGTH);
}

/*
 * @brief Converts the bytes of an IEEE 11073 floating point into an IEEE 754 float
 *
 * @param raw_11073 A pointer to the raw bytes that descirbe the IEEE 11073 float
 *
 * @return Returns a C float with the value contained in raw_11073
 */
float IEEE_11073_to_IEEE_754(IEEE_11073_FLOAT raw_11073) {
	int32_t significand;
	/* Build an IEEE float */

	/* Get the significand MSB aligned. This puts the sign bit in the MSB, which will be useful shortly. */
	significand = (int32_t) ((raw_11073 & 0x00FFFFFF) << 8);

	/*
	 * Right shift one byte over since we only have 3 bytes of significand.
	 * This copies the sign bit, which is why we wanted our data MSB aligned.
	 */
	significand >>=8;

	return ((float) significand) * pow(10.0,(float)((int8_t) (raw_11073>>24)));
}

#endif /* SRC_UTILS_C_ */
