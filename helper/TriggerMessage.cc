/*
 * TriggerMessage.cc
 *
 *  Created on: Dec 10, 2019
 *      Author: Kadir Yanık
 */

#include "TriggerMessage.h"

/*------------------------------------------------------------------------------*/
TriggerMessage::TriggerMessage(){
  usedNum = 0;
  offset = 0;
}

/*------------------------------------------------------------------------------*/
TriggerMessage::~TriggerMessage(){
}

/*------------------------------------------------------------------------------*/
int TriggerMessage::add(int id, double time){
  if(usedNum < EC_MAX_SYNC_SLOT_NUM){
    this->triggerMessageId[usedNum] = id;
    this->triggerMessageTime[usedNum] = time;
    
    usedNum++;
    return 0;
  }

  return -1;
}

/*------------------------------------------------------------------------------*/
int TriggerMessage::getUsedNum(){
  return this->usedNum;
}

/*------------------------------------------------------------------------------*/
int TriggerMessage::getOffset(){
  return this->offset;
}

/*------------------------------------------------------------------------------*/
int TriggerMessage::getId(int index){
  if(0 <= index && index < EC_MAX_SYNC_SLOT_NUM){
    return this->triggerMessageId[index];
  }

  return -1;
}

/*------------------------------------------------------------------------------*/
double TriggerMessage::getTime(int index){
  if(0 <= index && index < EC_MAX_SYNC_SLOT_NUM){
    return this->triggerMessageTime[index];
  }

  return -1;
}

/*------------------------------------------------------------------------------*/
int TriggerMessage::setOffset(int offset){
  this->offset = offset;
}

/*------------------------------------------------------------------------------*/
void TriggerMessage::clear(){
  this->usedNum = 0;
  this->offset = 0;
}