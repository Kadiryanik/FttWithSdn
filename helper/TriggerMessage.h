/*
 * TriggerMessage.h
 *
 *  Created on: Dec 10, 2019
 *      Author: Kadir Yanık
 */

#ifndef HELPER_TRIGGERMESSAGE_H_
#define HELPER_TRIGGERMESSAGE_H_

#include "conf.h"

class TriggerMessage {
public:
  TriggerMessage();
  virtual ~TriggerMessage();
  int add(int id, double time);
  int getUsedNum();
  int getOffset();
  int getId(int index);
  double getTime(int index);
  int setOffset(int offset);
  void clear();
private:
  int usedNum;
  int offset;
  int triggerMessageId[EC_MAX_SYNC_SLOT_NUM];
  double triggerMessageTime[EC_MAX_SYNC_SLOT_NUM];
};

#endif /* HELPER_TRIGGERMESSAGE_H_ */
