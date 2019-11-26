/*
 * FlowRules.h
 *
 *  Created on: Nov 26, 2019
 *      Author: Kadir Yanık
 */

#ifndef HELPER_FLOWRULES_H_
#define HELPER_FLOWRULES_H_

#include "Rule.h"

#define MAX_RULE_NUM 10 // TODO: could be dynamic via constructor

#define NEXT_DESTINATION_NOT_KNOWN 0xFF

class FlowRules {
private:
	Rule rules[MAX_RULE_NUM];
	uint8_t ruleOffset;
public:
    FlowRules();
    virtual ~FlowRules();
    uint8_t getNextDestination(uint8_t, uint8_t, uint8_t);
    uint8_t addRule(uint8_t, uint8_t, uint8_t, uint8_t);
};

#endif /* HELPER_FLOWRULES_H_ */
