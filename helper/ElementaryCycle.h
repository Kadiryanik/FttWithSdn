/*
 * ElementaryCycle.h
 *
 *  Created on: Dec 11, 2019
 *      Author: Kadir Yanık
 */

#ifndef HELPER_ELEMENTARYCYCLE_H_
#define HELPER_ELEMENTARYCYCLE_H_

#include <stdio.h>
#include "TriggerMessage.h"

class ElementaryCycle {
public:
  static ElementaryCycle* head;
  static ElementaryCycle* tail;
  static int elementaryCycleCount;

  ElementaryCycle();
  virtual ~ElementaryCycle();

  int elementaryCycleId;
  ElementaryCycle* next;
  TriggerMessage triggerMessage;
};

#endif /* HELPER_ELEMENTARYCYCLE_H_ */
