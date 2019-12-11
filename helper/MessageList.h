/*
 * MessageList.h
 *
 *  Created on: Dec 11, 2019
 *      Author: Kadir Yanık
 */

#ifndef HELPER_MESSAGELIST_H_
#define HELPER_MESSAGELIST_H_

#include "src/GeneralMessage_m.h"

#define ML_MSG_EXIST        0
#define ML_MSG_DOSENT_EXIST 1

/*------------------------------------------------------------------------------*/
class MessageListNode {
public:
  MessageListNode(GeneralMessage * gMsg);
  ~MessageListNode();

  MessageListNode* next;
  MessageListNode* prev;
  GeneralMessage* msg;
};

/*------------------------------------------------------------------------------*/
class MessageList {
  MessageListNode* head;
  int messageCount;
public:
  MessageList();
  ~MessageList();
  int getMessageCount();
  void add(GeneralMessage *gMsg);
  int check(int messageId);
  GeneralMessage* get(int messageId);
};

#endif /* HELPER_MESSAGELIST_H_ */
