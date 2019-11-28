/*
 * Rule.h
 *
 *  Created on: Nov 26, 2019
 *      Author: Kadir Yanık
 */

#ifndef HELPER_CONF_H_
#define HELPER_CONF_H_

/*------------------------------------------------------------------------------*/
#define GM_TYPE_DATA          0x01
#define GM_TYPE_FLOW_RULE     0x02
#define GM_TYPE_ADVERTISE     0x03
#define GM_TYPE_ADVERTISE_ACK 0x04

/*------------------------------------------------------------------------------*/
#define CONTROLLER_ID    0
#define CONTROLLER_INDEX 0

/*------------------------------------------------------------------------------*/
#define TOTAL_SWITCH_NUM 4 // TODO: must update when topology changed

#define SWITCH_ID_OFFSET    10
#define SWITCH_INDEX_OFFSET (SWITCH_ID_OFFSET - 1)

#define SWITCH_0_ID   (SWITCH_ID_OFFSET)
#define SWITCH_1_ID   (SWITCH_0_ID + 1)
#define SWITCH_2_ID   (SWITCH_1_ID + 1)
#define SWITCH_3_ID   (SWITCH_2_ID + 1)

#define SWITCH_0_INDEX (SWITCH_0_ID - SWITCH_INDEX_OFFSET)
#define SWITCH_1_INDEX (SWITCH_1_ID - SWITCH_INDEX_OFFSET)
#define SWITCH_2_INDEX (SWITCH_2_ID - SWITCH_INDEX_OFFSET)
#define SWITCH_3_INDEX (SWITCH_3_ID - SWITCH_INDEX_OFFSET)

/*------------------------------------------------------------------------------*/
#define EXPECTED_GATE_MAX_NUM 10
#define NULL_GATE_VAL 0xFF

#endif /* HELPER_CONF_H_ */