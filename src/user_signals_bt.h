/*
 * @file user_signals_bt.h
 * @brief Defines the core message types for internal signaling of state changes.
 *
 * @author John-Michael O'Brien
 * @date Oct 15, 2018
 */

#ifndef SRC_USER_SIGNALS_BT_H_
#define SRC_USER_SIGNALS_BT_H_

#define PB_EVT_0 (1<<1)
#define PB_EVT_1 (1<<2)

#define CORE_EVT_BOOT (1<<4)
#define CORE_EVT_RESET (1<<5)
#define CORE_EVT_POST_BOOT (1<<6) /* Do not assume that this will always come before the provisioned event! */
#define CORE_EVT_NETWORK_READY (1<<7)


#endif /* SRC_USER_SIGNALS_BT_H_ */
