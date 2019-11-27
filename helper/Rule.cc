/*
 * Rule.cpp
 *
 *  Created on: Nov 26, 2019
 *      Author: Kadir Yanık
 */

#include "Rule.h"

Rule::Rule() {
    this->source = 0;
    this->destination = 0;
    this->port = 0;
    this->nextDestination = 0;
}

Rule::~Rule() {
}

int Rule::checkRule(int src, int dst, int p) {
	if(this->source == src && this->destination == dst && this->port == p){
		return this->nextDestination;
	}
	return RULE_NO_MATCH;
}

void Rule::setRule(int src, int dst, int p, int next) {
    this->source = src;
    this->destination = dst;
    this->port = p;
    this->nextDestination = next;
}
