/*
 * FlowRules.cpp
 *
 *  Created on: Nov 26, 2019
 *      Author: Kadir Yanık
 */

#include "FlowRules.h"

FlowRules::FlowRules() {
	this->ruleOffset = 0;
}

FlowRules::~FlowRules() {
}

uint8_t FlowRules::getNextDestination(uint8_t src, uint8_t dst, uint8_t p) {
	uint8_t i;
	for(i = 0; i < this->ruleOffset; i++){
		uint8_t ret = this->rules[i].checkRule(src, dst, p);
		if(ret != RULE_NO_MATCH){
			// we found matching rule
			return ret;
		}
	}

	return NEXT_DESTINATION_NOT_KNOWN;
}

uint8_t FlowRules::addRule(uint8_t src, uint8_t dst, uint8_t p, uint8_t next) {
	if(this->ruleOffset < MAX_RULE_NUM){
		this->rules[this->ruleOffset].setRule(src, dst, p, next);
		this->ruleOffset++;
	} else{
		// TODO: print some info
	}
}
