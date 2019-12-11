/*
 * MessageList.cc
 *
 *  Created on: Dec 11, 2019
 *      Author: Kadir Yanık
 */

#include "MessageList.h"

/*------------------------------------------------------------------------------*/
MessageListNode::MessageListNode(GeneralMessage * gMsg){
  this->next = NULL;
  this->prev = NULL;
  this->msg = gMsg;
}

/*------------------------------------------------------------------------------*/
MessageListNode::~MessageListNode(){
}

/*------------------------------------------------------------------------------*/
MessageList::MessageList(){
  this->head = NULL;
  this->messageCount = 0;
}

/*------------------------------------------------------------------------------*/
MessageList::~MessageList(){
  MessageListNode* p = this->head;
  while(p != NULL){
    delete p->msg;
    p = p->next;
  }
}

/*------------------------------------------------------------------------------*/
int MessageList::getMessageCount(){
  return this->messageCount;
}

/*------------------------------------------------------------------------------*/
void MessageList::add(GeneralMessage *gMsg){
  MessageListNode* ptr = this->head;
  
  if(ptr == NULL){
    this->head = new MessageListNode(gMsg);
  } else{
    while(ptr->next != NULL){
      ptr = ptr->next;
    }
    ptr->next = new MessageListNode(gMsg);
    ptr->next->prev = ptr;
  }

  this->messageCount++;
}

/*------------------------------------------------------------------------------*/
int MessageList::check(int messageId){
  if(this->head == NULL){
    return ML_MSG_DOSENT_EXIST;
  }

  MessageListNode* ptr = this->head;
  while(ptr->next != NULL){
    if(ptr->msg->getMessageId() == messageId){
      return ML_MSG_EXIST; // we have this message
    }

    ptr = ptr->next;
  }
  return ML_MSG_DOSENT_EXIST;
}

/*------------------------------------------------------------------------------*/
GeneralMessage* MessageList::get(int messageId){
  GeneralMessage* msg = NULL;

  MessageListNode* ptr = this->head;
  while(ptr->next != NULL){
    if(ptr->msg->getMessageId() == messageId){
      msg = ptr->msg;
      break;
    }

    ptr = ptr->next;
  }
  if(msg != NULL){
    if(ptr == this->head && this->messageCount == 1){
      this->head = NULL;
    } else{
      // remove it
      if(ptr->prev != NULL){
        ptr->prev->next = ptr->next;
      } else{ // prev == NULL mean next one is new head
        this->head = ptr->next;
      }
      if(ptr->next != NULL){
        ptr->next->prev = ptr->prev;
      }
    }
    // clean up
    delete ptr;

    this->messageCount--;
  }
  // return msg
  return msg;
}
