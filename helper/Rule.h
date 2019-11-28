/*
 * Rule.h
 *
 *  Created on: Nov 26, 2019
 *      Author: Kadir Yanık
 */

#ifndef HELPER_RULE_H_
#define HELPER_RULE_H_

#include <stdio.h>

#include "conf.h"

#define RULE_NO_MATCH 0xFF

class Rule {
private:
  int source;
  int destination;
  int port;
  int nextDestination;
public:
  Rule();
  virtual ~Rule();
  int checkRule(int, int, int);
  void setRule(int, int, int, int);
};

#endif /* HELPER_RULE_H_ */
