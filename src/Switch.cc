/*
 * Switch.cc
 *
 *  Created on: Nov 26, 2019
 *      Author: Kadir Yanık
 */

#include <stdio.h>
#include <string.h>
#include <omnetpp.h>

#include "helper/FlowRules.h"
#include "helper/Relation.h"
#include "helper/ElementaryCycle.h"
#include "helper/MessageList.h"

using namespace omnetpp;

/*------------------------------------------------------------------------------*/
// TODO: use omnet logging features to seperate error, warn and info logs

/*------------------------------------------------------------------------------*/
class Switch : public cSimpleModule
{
public:
  Switch();
  virtual ~Switch();
protected:
  virtual void forwardMessage(GeneralMessage *gMsg);
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
private:
  GeneralMessage *generateMesagge(int type, int messageId, int dest, int port, char *data);
  void sendAdv(int gateNum);
  void sendAdvAck(int gateNum);
  void setGateInfoFields(GeneralMessage *gMsg, int *gateInfo, int gateNum);
  void setCurrentTriggerMessage(GeneralMessage *gMsg);
  void checkAndSchedule();

  cMessage *timerEvent;
  cMessage *trafficGenerator;
  cMessage *timerTrigger;

  FlowRules flowRules;
  int advDone;
  int defaultRoute;
  int id;
  int replied; // TODO: make this more readable
  int gateToDest[EXPECTED_GATE_MAX_NUM];
  MessageList messageList;
  TriggerMessage* currentTriggerMessage;
  int seqNum;
  int debugMessageListCount;
};

Define_Module(Switch);

/*------------------------------------------------------------------------------*/
Switch::Switch()
{
  timerEvent = NULL;
  advDone = 0;

  int i;
  for(i = 0; i < EXPECTED_GATE_MAX_NUM; i++){
    gateToDest[i] = NULL_GATE_VAL;
  }

  currentTriggerMessage = NULL;
  seqNum = 0;
}
/*------------------------------------------------------------------------------*/
Switch::~Switch()
{
  cancelAndDelete(timerEvent);
  cancelAndDelete(trafficGenerator);
  cancelAndDelete(timerTrigger);
}

/*------------------------------------------------------------------------------*/
void Switch::initialize()
{
  // parse values for each switch from omnetpp.ini file
  // we actually select the gate[0] to shortest path to controller for each switch
  defaultRoute = par("defaultRoute");

  // get id, too
  id = par("id");

  WATCH(debugMessageListCount);

  // init pointer, schedule when tm received
  timerTrigger = new cMessage("trigger");

  // set timer to start advertisement
  timerEvent = new cMessage("event");
  scheduleAt(simTime() + 0.1, timerEvent);
}

/*------------------------------------------------------------------------------*/
void Switch::handleMessage(cMessage *msg)
{
  if(msg == timerEvent){
    if(!advDone){
      int i, n = gateSize("gate");
      for(i = 0; i < n; i++){
        if(gateToDest[i] == NULL_GATE_VAL){
          sendAdv(i);
          scheduleAt(simTime() + uniform(0, 1), timerEvent);
          return;
        }
      }
      // we reach here means that all gate-destination relationships set
      advDone = 1;
      EV << "handleMessage: Advertising done!\n";
    }

    // schedule the GATE_INFO message to send controller
    GeneralMessage *gMsg = generateMesagge(GM_TYPE_GATE_INFO, 0, 0, 0, (char *)"GATE_INFO");
    setGateInfoFields(gMsg, gateToDest, gateSize("gate"));
    scheduleAt(simTime() + 0.1, gMsg);

    /* TODO: we assume it was forwarded to CONTROLLER, may ACK can be added.
     * Repeat the message until we receive an ACK.
     * scheduleAt(simTime() + 0.5, timerEvent); */

    trafficGenerator = new cMessage("trafficGenerator");
    scheduleAt(simTime() + 0.1, trafficGenerator);
  } else if(msg == trafficGenerator){
    GeneralMessage* message;
    char data[20];
    if(id == SWITCH_0_ID){
      sprintf(data, "MSG_0 %d", seqNum++);
      message = generateMesagge(GM_TYPE_DATA, MESSAGE_ID_0, 0, 0, data);
      messageList.add(message);
      scheduleAt(simTime() + 0.2, trafficGenerator); // 5 message per sec
    } else if(id == SWITCH_1_ID){
      sprintf(data, "MSG_1 %d", seqNum++);
      for(int i = 0; i < 10; i++){
        message = generateMesagge(GM_TYPE_DATA, MESSAGE_ID_1, 0, 0, data);
        messageList.add(message);
      }
      scheduleAt(simTime() + 5, trafficGenerator); // 2 message per sec
    } else if(id == SWITCH_2_ID){
      sprintf(data, "MSG_2 %d", seqNum++);
      message = generateMesagge(GM_TYPE_DATA, MESSAGE_ID_2, 0, 0, data);
      messageList.add(message);
      scheduleAt(simTime() + 1, trafficGenerator); // 1 message per sec
    } else{
      cancelEvent(trafficGenerator);
    }
  } else if(msg == timerTrigger){
    GeneralMessage* message = messageList.get(currentTriggerMessage->getId(currentTriggerMessage->getOffset() - 1)); // -1 to get correct offset
    this->debugMessageListCount = messageList.getMessageCount();
    //EV << "MessageCount = " << messageList.getMessageCount() << "\n";
    forwardMessage(message);
    checkAndSchedule();
  } else{
    GeneralMessage *gMsg = check_and_cast<GeneralMessage *>(msg);
    if(gMsg == NULL){
      EV << "handleMessage: Message cast failed!\n";
      return;
    }

    // destination area is not valid for advertisement messages so check them first
    int type = gMsg->getType();
    if(type == GM_TYPE_ADVERTISE){
      EV << "handleMessage: Adv message received from ";
      printIndexInHR(getIndexFromId(gMsg->getSource()), 1);

      int gate = msg->getArrivalGate()->getIndex();
      gateToDest[gate] = gMsg->getSource();
      sendAdvAck(gate);

      delete msg;
    } else if(type == GM_TYPE_ADVERTISE_ACK){
      // set gate-destination relation
      gateToDest[msg->getArrivalGate()->getIndex()] = gMsg->getSource();

      delete msg;
#if WITH_ADMISSION_CONTROL
    } else if(type != GM_TYPE_ADMISSION && gMsg->getDestination() == id) { // forward ADMISSION messages even for us
#else /* WITH_ADMISSION_CONTROL */
    } else if(gMsg->getDestination() == id) { // check is message for us
#endif /* WITH_ADMISSION_CONTROL */
      if(type == GM_TYPE_DATA){
        EV << "handleMessage: Message " << msg << " received from "; 
        printIndexInHR(getIndexFromId(gMsg->getSource()), 0);
      } else if(type == GM_TYPE_FLOW_RULE){
        EV << "handleMessage: FlowRule received from ";
        printIndexInHR(getIndexFromId(gMsg->getSource()), 1);

        flowRules.addRule(gMsg->getFRuleSource(), gMsg->getFRuleDestination(), gMsg->getFRulePort(), gMsg->getFRuleNextDestination());
      } else{
        EV << "handleMessage: Unknown message type!\n";
      }

      delete msg; // free received msg
    } else if(type == GM_TYPE_TM){
      setCurrentTriggerMessage(gMsg);
      checkAndSchedule();

      delete msg;
    } else {
      forwardMessage(gMsg);
    }
  }
}

/*------------------------------------------------------------------------------*/
void Switch::forwardMessage(GeneralMessage *gMsg)
{
  int i, n = gateSize("gate");
  int gateNum = n;
  int dest = flowRules.getNextDestination(gMsg->getSource(), gMsg->getDestination(), gMsg->getPort());

  if(gMsg->getType() == GM_TYPE_FLOW_RULE){
    unsigned int arrSize = gMsg->getNextHopArraySize();

    if(arrSize > 0){
      // remove us from list and forward to nextHop
      for(i = 0; i < arrSize - 1; i++){
        gMsg->setNextHop(i, gMsg->getNextHop(i + 1));
      }

      // manipulate the dest variable for forwarding GM_TYPE_FLOW_RULE messages
      dest = gMsg->getNextHop(0); // points to next destination

      // set new array size, not including us
      gMsg->setNextHopArraySize(arrSize - 1);
    } // else actually didin't expect, last hop points to destination, so will not try to forward
  }

  // get destination's gate from destination id
  for(i = 0; i < n; i++){
    if(gateToDest[i] == dest){
      gateNum = i;
      break;
    }
  }

  if(gateNum >= n){
#if WITH_ADMISSION_CONTROL
    if(gMsg->getType() == GM_TYPE_DATA && gMsg->getSource() == id){ // if we are sending data and doesn't match any rules 
      gMsg->setType(GM_TYPE_ADMISSION); // manipulate the type to ensure that the message is forwarded to the controller
    }
#endif /* WITH_ADMISSION_CONTROL */
    if(gMsg->getType() == GM_TYPE_FLOW_RULE){
      EV << "forwardMessage: FATAL! (nextHop is ";
      printIndexInHR(getIndexFromId(dest), 0);
      EV << " and not connected to our gates!\n";
    }
    gateNum = defaultRoute;
  }

  EV << "forwardMessage: Forwarding message [" << gMsg->getMessageId() << "] on gate[" << gateNum << "]\n";
  // $o and $i suffix is used to identify the input/output part of a two way gate
  send(gMsg, "gate$o", gateNum);
}

/*------------------------------------------------------------------------------*/
GeneralMessage* Switch::generateMesagge(int type, int messageId, int dest, int port, char *data)
{
  GeneralMessage *gMsg = new GeneralMessage(data);
  if(gMsg == NULL){
    return NULL;
  }

  gMsg->setType(type);
  gMsg->setMessageId(messageId);
  gMsg->setSource(id);
  gMsg->setDestination(dest);
  gMsg->setPort(port);

  return gMsg;
}

/*------------------------------------------------------------------------------*/
void Switch::sendAdv(int gateNum)
{
  GeneralMessage *gMsg = generateMesagge(GM_TYPE_ADVERTISE, 0, 0, 0, (char *)"ADV");
  if(gMsg == NULL){
    EV << "sendAdv: Message generate failed!\n";
    return;
  }

  EV << "sendAdv: Sending to gate[" << gateNum << "]\n";
  send(gMsg, "gate$o", gateNum);
}

/*------------------------------------------------------------------------------*/
void Switch::sendAdvAck(int gateNum)
{
  GeneralMessage *gMsg = generateMesagge(GM_TYPE_ADVERTISE_ACK, 0, 0, 0, (char *)"ADV-ACK");
  if(gMsg == NULL){
    EV << "sendAdvAck: Message generate failed!\n";
    return;
  }

  EV << "sendAdvAck: Sending to gate[" << gateNum << "]\n";
  send(gMsg, "gate$o", gateNum);
}

/*------------------------------------------------------------------------------*/
void Switch::setGateInfoFields(GeneralMessage *gMsg, int *gateInfo, int gateNum)
{
  gMsg->setGateInfoArraySize(gateNum);
  for(int i = 0; i < gateNum; i++){
    gMsg->setGateInfo(i, gateInfo[i]);
  }
}

/*------------------------------------------------------------------------------*/
void Switch::checkAndSchedule(){
  for(int i = currentTriggerMessage->getOffset(); i < currentTriggerMessage->getUsedNum(); i++){
    if(messageList.check(currentTriggerMessage->getId(i)) == ML_MSG_EXIST){
      EV << "[" << currentTriggerMessage->getId(i) << "->" << currentTriggerMessage->getTime(i) << "] scheduled!\n";
      currentTriggerMessage->setOffset(i + 1); // point next one
      scheduleAt(currentTriggerMessage->getTime(i), timerTrigger);
      return;
    }
  }
}

/*------------------------------------------------------------------------------*/
void Switch::setCurrentTriggerMessage(GeneralMessage *gMsg){
  if(currentTriggerMessage == NULL){
    currentTriggerMessage = new TriggerMessage();
  } else{
    currentTriggerMessage->clear();
  }

  EV << "setCurrentTriggerMessage:";
  for(int i = 0; i < gMsg->getTriggerMessageIdArraySize(); i++){
    currentTriggerMessage->add(gMsg->getTriggerMessageId(i), gMsg->getTriggerMessageTime(i));

    EV << " [" << currentTriggerMessage->getId(i) << "->" << currentTriggerMessage->getTime(i) << "]";
  }
  EV << "\n";
}
