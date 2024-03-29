/*
 * FlowRules.h
 *
 *  Created on: Nov 26, 2019
 *      Author: Kadir Yanık
 */

#ifndef HELPER_FLOWRULES_H_
#define HELPER_FLOWRULES_H_

#include "Rule.h"

#define MAX_RULE_NUM 20 // TODO: could be dynamic via constructor

class FlowRules {
private:
  Rule rules[MAX_RULE_NUM];
  int ruleOffset;
public:
  FlowRules();
  virtual ~FlowRules();
  int getNextDestination(int, int, int);
  void addRule(int, int, int, int);
};

#endif /* HELPER_FLOWRULES_H_ */
