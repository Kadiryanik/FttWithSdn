/*
 * FlowRules.cpp
 *
 *  Created on: Nov 26, 2019
 *      Author: Kadir Yanık
 */

#include "FlowRules.h"

/*------------------------------------------------------------------------------*/
FlowRules::FlowRules() {
  this->ruleOffset = 0;
}

/*------------------------------------------------------------------------------*/
FlowRules::~FlowRules() {
}

/*------------------------------------------------------------------------------*/
int FlowRules::getNextDestination(int src, int dst, int p) {
  int i;
  for(i = 0; i < this->ruleOffset; i++){
    int ret = this->rules[i].checkRule(src, dst, p);
    if(ret != RULE_NO_MATCH){
      // we found matching rule
      return ret;
    }
  }

  return DESTINATION_NOT_FOUND;
}

/*------------------------------------------------------------------------------*/
void FlowRules::addRule(int src, int dst, int p, int next) {
  if(this->ruleOffset < MAX_RULE_NUM){
    this->rules[this->ruleOffset].setRule(src, dst, p, next);
    this->ruleOffset++;
  } else{
    // TODO: print some info
  }
}
