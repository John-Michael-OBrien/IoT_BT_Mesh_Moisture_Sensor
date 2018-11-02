/*
 * mesh_utils.h
 *
 *  Created on: Nov 1, 2018
 *      Author: jtc-b
 */

#ifndef SRC_MESH_UTILS_H_
#define SRC_MESH_UTILS_H_

#define MESH_PROV_BEACON_USE_ADV (1<<0)
#define MESH_PROV_BEACON_USE_GATT (1<<1)

#define MESH_PROV_NO_PUBLIC_KEY (0)
#define MESH_PROV_AUTH_METHOD_NO_OOB (1<<0)
#define MESH_PROV_AUTH_METHOD_STATIC_OOB (1<<1)
#define MESH_PROV_AUTH_METHOD_INPUT_OOB (1<<2)
#define MESH_PROV_AUTH_METHOD_OUTPUT_OOB (1<<3)

#define MESH_PROV_OOB_DISPLY_BLINK (0x00)
#define MESH_PROV_OOB_DISPLY_BEEP (0x01)
#define MESH_PROV_OOB_DISPLY_VIBRATE (0x02)
#define MESH_PROV_OOB_DISPLY_NUMERIC (0x03)
#define MESH_PROV_OOB_DISPLY_ALPHANUMERIC (0x04)

/*
 * LOTS of undefined constants. The secret to finding this is in the forum article at:
 * https://www.silabs.com/community/wireless/bluetooth/forum.topic.html/mesh_oob_authenticat-h2Ec
 */

/* This isn't in the BGAPI docs, but comes up in section 5.4.1.2 of the Mesh Profile.  */
#define MESH_PROV_OOB_OUTPUT_ACTIONS_NONE (0)
#define MESH_PROV_OOB_OUTPUT_ACTIONS_BLINK (1<<0)
#define MESH_PROV_OOB_OUTPUT_ACTIONS_BEEP (1<<1)
#define MESH_PROV_OOB_OUTPUT_ACTIONS_VIBRATE (1<<2)
#define MESH_PROV_OOB_OUTPUT_ACTIONS_NUMERIC (1<<3)
#define MESH_PROV_OOB_OUTPUT_ACTIONS_ALPHANUMERIC (1<<4)
#define MESH_PROV_OOB_INPUT_ACTIONS_NONE (0)
#define MESH_PROV_OOB_INPUT_ACTIONS_PUSH (1<<0)
#define MESH_PROV_OOB_INPUT_ACTIONS_TWIST (1<<1)
#define MESH_PROV_OOB_INPUT_ACTIONS_NUMERIC (1<<2)
#define MESH_PROV_OOB_INPUT_ACTIONS_ALPHANUMERIC (1<<3)

/* This ALSO isn't in the BGAPI docs, but comes up as part of 3.9.2 of the Mesh Profile */
#define MESH_PROV_OOB_LOCATION_OTHER (1<<0)
#define MESH_PROV_OOB_LOCATION_URL (1<<1)
#define MESH_PROV_OOB_LOCATION_2D_BAR_CODE (1<<2)
#define MESH_PROV_OOB_LOCATION_BAR_CODE (1<<3)
#define MESH_PROV_OOB_LOCATION_NFC (1<<4)
#define MESH_PROV_OOB_LOCATION_NUMBER (1<<5)
#define MESH_PROV_OOB_LOCATION_STRING (1<<6)
#define MESH_PROV_OOB_LOCATION_ON_BOX (1<<11)
#define MESH_PROV_OOB_LOCATION_IN_BOX (1<<12)
#define MESH_PROV_OOB_LOCATION_ON_PAPER (1<<13)
#define MESH_PROV_OOB_LOCATION_INSIDE_MANUAL (1<<14)
#define MESH_PROV_OOB_LOCATION_ON_DEVICE (1<<15)

#endif /* SRC_MESH_UTILS_H_ */
