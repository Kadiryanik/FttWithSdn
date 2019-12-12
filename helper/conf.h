/*
 * Rule.h
 *
 *  Created on: Nov 26, 2019
 *      Author: Kadir Yanık
 */

#ifndef HELPER_CONF_H_
#define HELPER_CONF_H_

/*------------------------------------------------------------------------------*/
#define WITH_ADMISSION_CONTROL 1

/*------------------------------------------------------------------------------*/
#define GM_TYPE_DATA          0x01
#define GM_TYPE_FLOW_RULE     0x02
#define GM_TYPE_ADVERTISE     0x03
#define GM_TYPE_ADVERTISE_ACK 0x04
#if WITH_ADMISSION_CONTROL
#define GM_TYPE_ADMISSION     0x05
#endif /* WITH_ADMISSION_CONTROL */
#define GM_TYPE_GATE_INFO     0x06
#define GM_TYPE_TM            0x07

/*------------------------------------------------------------------------------*/
#define CONTROLLER_ID    0
#define CONTROLLER_INDEX 0
/*------------------------------------------------------------------------------*/
#define CONTROLLER_CONNECTED_ALL_DIRECTLY 1

/*------------------------------------------------------------------------------*/
#define BROADCAST_ID  0xFF

/*------------------------------------------------------------------------------*/
#define TOTAL_SWITCH_NUM 5 // TODO: must update when topology changed

#define SWITCH_ID_OFFSET    10
#define SWITCH_INDEX_OFFSET (SWITCH_ID_OFFSET - 1)

#define SWITCH_0_ID   (SWITCH_ID_OFFSET)
#define SWITCH_1_ID   (SWITCH_0_ID + 1)
#define SWITCH_2_ID   (SWITCH_1_ID + 1)
#define SWITCH_3_ID   (SWITCH_2_ID + 1)
#define SWITCH_4_ID   (SWITCH_3_ID + 1)

#define SWITCH_0_INDEX (SWITCH_0_ID - SWITCH_INDEX_OFFSET)
#define SWITCH_1_INDEX (SWITCH_1_ID - SWITCH_INDEX_OFFSET)
#define SWITCH_2_INDEX (SWITCH_2_ID - SWITCH_INDEX_OFFSET)
#define SWITCH_3_INDEX (SWITCH_3_ID - SWITCH_INDEX_OFFSET)
#define SWITCH_4_INDEX (SWITCH_4_ID - SWITCH_INDEX_OFFSET)

/*------------------------------------------------------------------------------*/
#define MESSAGE_ID_0          0xF0
#define MESSAGE_ID_1          0xF1
#define MESSAGE_ID_2          0xF2
#define MESSAGE_ID_3          MESSAGE_ID_0
#define MESSAGE_ID_4          MESSAGE_ID_0
#define MESSAGE_ID_5          MESSAGE_ID_1

#define MESSAGE_ID_0_ENABLED 1
#define MESSAGE_ID_1_ENABLED 1
#define MESSAGE_ID_2_ENABLED 1
#define MESSAGE_ID_3_ENABLED 1
#define MESSAGE_ID_4_ENABLED 1
#define MESSAGE_ID_5_ENABLED 1

#define MESSAGE_ID_0_SRC      SWITCH_0_ID
#define MESSAGE_ID_0_DEST     SWITCH_1_ID
#define MESSAGE_ID_0_PORT     0
#define MESSAGE_ID_0_PER_SEC  5

#define MESSAGE_ID_1_SRC      SWITCH_2_ID
#define MESSAGE_ID_1_DEST     CONTROLLER_ID
#define MESSAGE_ID_1_PORT     0
#define MESSAGE_ID_1_PER_SEC  2

#define MESSAGE_ID_2_SRC      SWITCH_1_ID
#define MESSAGE_ID_2_DEST     SWITCH_3_ID
#define MESSAGE_ID_2_PORT     0
#define MESSAGE_ID_2_PER_SEC  1

#define MESSAGE_ID_3_SRC      SWITCH_3_ID
#define MESSAGE_ID_3_DEST     SWITCH_2_ID
#define MESSAGE_ID_3_PORT     0
#define MESSAGE_ID_3_PER_SEC  5

#define MESSAGE_ID_4_SRC      SWITCH_4_ID
#define MESSAGE_ID_4_DEST     SWITCH_2_ID
#define MESSAGE_ID_4_PORT     0
#define MESSAGE_ID_4_PER_SEC  5

#define MESSAGE_ID_5_SRC      SWITCH_2_ID
#define MESSAGE_ID_5_DEST     SWITCH_0_ID
#define MESSAGE_ID_5_PORT     0
#define MESSAGE_ID_5_PER_SEC  1

/*------------------------------------------------------------------------------*/
#define EXPECTED_GATE_MAX_NUM 10
#define DESTINATION_NOT_FOUND 0xFD
#define NULL_GATE_VAL         0xFF

/*------------------------------------------------------------------------------*/
#define EC_TOTAL_SLOT_NUM          10
#define EC_MAX_SYNC_SLOT_NUM       6
#define EC_EACH_SLOT_LEN_IN_SEC    (0.025)

/*------------------------------------------------------------------------------*/
#define CHANNEL_DELAYS             0.02

#endif /* HELPER_CONF_H_ */