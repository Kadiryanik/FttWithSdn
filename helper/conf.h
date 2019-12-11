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
#define MESSAGE_ID_0          0xF0
#define MESSAGE_ID_1          0xF1
#define MESSAGE_ID_2          0xF2

/*------------------------------------------------------------------------------*/
#define CONTROLLER_ID    0
#define CONTROLLER_INDEX 0

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
#define EXPECTED_GATE_MAX_NUM 10
#define DESTINATION_NOT_FOUND 0xFD
#define NULL_GATE_VAL         0xFF

/*------------------------------------------------------------------------------*/
#define EC_TOTAL_SLOT_NUM          10
#define EC_MAX_SYNC_SLOT_NUM       6
#define EC_EACH_SLOT_LEN_IN_SEC    (0.025)

#endif /* HELPER_CONF_H_ */