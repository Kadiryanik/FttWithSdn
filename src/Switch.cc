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
#include "GeneralMessage_m.h"

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
  GeneralMessage *generateMesagge(int type, int dest, int port, char *data);
  void sendAdv(int gateNum);
  void sendAdvAck(int gateNum);
  void setGateInfoFields(GeneralMessage *gMsg, int *gateInfo, int gateNum);

  cMessage *timerEvent;
  FlowRules flowRules;
  int advDone;
  int defaultRoute;
  int id;
  int replied; // TODO: make this more readable
  int gateToDest[EXPECTED_GATE_MAX_NUM];
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
}
/*------------------------------------------------------------------------------*/
Switch::~Switch()
{
  cancelAndDelete(timerEvent);
}

/*------------------------------------------------------------------------------*/
void Switch::initialize()
{
  // parse values for each switch from omnetpp.ini file
  // we actually select the gate[0] to shortest path to controller for each switch
  defaultRoute = par("defaultRoute");

  // get id, too
  id = par("id");

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
    GeneralMessage *gMsg = generateMesagge(GM_TYPE_GATE_INFO, 0, 0, (char *)"GATE_INFO");
    setGateInfoFields(gMsg, gateToDest, gateSize("gate"));
    scheduleAt(simTime() + 0.1, gMsg);

    /* TODO: we assume it was forwarded to CONTROLLER, may ACK can be added.
     * Repeat the message until we receive an ACK.
     * scheduleAt(simTime() + 5, timerEvent); */
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
    } else if(type == GM_TYPE_ADVERTISE_ACK){
      // set gate-destination relation
      gateToDest[msg->getArrivalGate()->getIndex()] = gMsg->getSource();
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
      EV << "TM received!\n";
      for(int i = 0; i < gMsg->getTriggerMessageIdArraySize(); i++){
        EV << " msgId: " << gMsg->getTriggerMessageId(i) << " -> " << gMsg->getTriggerMessageTime(i) << "\n";
      }

      delete gMsg;
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

  EV << "forwardMessage: Forwarding message on gate[" << gateNum << "]\n";
  // $o and $i suffix is used to identify the input/output part of a two way gate
  send(gMsg, "gate$o", gateNum);
}

/*------------------------------------------------------------------------------*/
GeneralMessage* Switch::generateMesagge(int type, int dest, int port, char *data)
{
  GeneralMessage *gMsg = new GeneralMessage(data);
  if(gMsg == NULL){
    return NULL;
  }

  gMsg->setType(type);
  gMsg->setSource(id);
  gMsg->setDestination(dest);
  gMsg->setPort(port);

  return gMsg;
}

/*------------------------------------------------------------------------------*/
void Switch::sendAdv(int gateNum)
{
  GeneralMessage *gMsg = generateMesagge(GM_TYPE_ADVERTISE, 0, 0, (char *)"ADV");
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
  GeneralMessage *gMsg = generateMesagge(GM_TYPE_ADVERTISE_ACK, 0, 0, (char *)"ADV-ACK");
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
