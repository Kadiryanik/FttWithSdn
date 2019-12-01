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

  replied = 0;

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
    if(id == SWITCH_2_ID && !(replied & (1 << SWITCH_3_INDEX))){
      // initilize to send data to controller
      char data[] = "DATA-sw2-to-sw3";
      GeneralMessage *gMsg = generateMesagge(GM_TYPE_DATA, SWITCH_3_ID, 0, data);
      scheduleAt(simTime() + 0.1, gMsg);

      // schedule periodicly
      scheduleAt(simTime() + 5, timerEvent);
    } else if(id == SWITCH_0_ID && !(replied & (1 << SWITCH_1_INDEX))){
      // initilize to send data to controller
      char data[] = "DATA-sw0-to-sw1";
      GeneralMessage *gMsg = generateMesagge(GM_TYPE_DATA, SWITCH_1_ID, 0, data);
      scheduleAt(simTime() + 0.1, gMsg);

      // schedule periodicly
      scheduleAt(simTime() + 5, timerEvent);
    } else if(id == SWITCH_4_ID && !(replied & (1 << SWITCH_0_INDEX))){
      // initilize to send data to controller
      char data[] = "DATA-sw4-to-sw0";
      GeneralMessage *gMsg = generateMesagge(GM_TYPE_DATA, SWITCH_0_ID, 0, data);
      scheduleAt(simTime() + 0.1, gMsg);

      // schedule periodicly
      scheduleAt(simTime() + 5, timerEvent);
    }
  } else{
    GeneralMessage *gMsg = check_and_cast<GeneralMessage *>(msg);
    if(gMsg == NULL){
      EV << "handleMessage: Message cast failed!\n";
      return;
    }

    // destination area is not valid for advertisement messages so check them first
    int type = gMsg->getType();
    if(type == GM_TYPE_ADVERTISE){
      int srcIndex = getIndexFromId(gMsg->getSource());
      if(srcIndex == CONTROLLER_INDEX){
        EV << "handleMessage: Adv message received from Controller\n";
      } else{
        EV << "handleMessage: Adv message received from Switch_" << (srcIndex - 1) << "\n";
      }

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
        int srcIndex = getIndexFromId(gMsg->getSource());
        if(srcIndex == CONTROLLER_INDEX){
          EV << "handleMessage: Message " << msg << " received from Controller, send back\n";
        } else{
          EV << "handleMessage: Message " << msg << " received from Switch_" << (srcIndex - 1) << ", send back\n";
        }

        char data[] = "HELLO-BACK"; // TODO: add src-seq count in message
        GeneralMessage *reply = generateMesagge(GM_TYPE_DATA, gMsg->getSource(), 0, data);
        scheduleAt(simTime() + 0.5, reply);

        replied |= (1 << getIndexFromId(gMsg->getSource())); // set flag to cancelEvent
      } else if(type == GM_TYPE_FLOW_RULE){
        EV << "handleMessage: FlowRule received from ID:" << gMsg->getSource() << "\n";
        flowRules.addRule(gMsg->getFRuleSource(), gMsg->getFRuleDestination(), gMsg->getFRulePort(), gMsg->getFRuleNextDestination());
      } else{
        EV << "handleMessage: Unknown message type!\n";
      }

      delete msg; // free received msg
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
      int srcIndex = getIndexFromId(dest);
        if(srcIndex == CONTROLLER_INDEX){
          EV << "forwardMessage: FATAL! (nextHop is Controller and not connected to our gates!\n";
        } else{
          EV << "forwardMessage: FATAL! (nextHop is Switch_" << (srcIndex - 1) << " connected to our gates!\n";
        }
      
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
  GeneralMessage *gMsg = generateMesagge(GM_TYPE_ADVERTISE, 0, 0, NULL);
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
  GeneralMessage *gMsg = generateMesagge(GM_TYPE_ADVERTISE_ACK, 0, 0, NULL);
  if(gMsg == NULL){
    EV << "sendAdvAck: Message generate failed!\n";
    return;
  }

  EV << "sendAdvAck: Sending to gate[" << gateNum << "]\n";
  send(gMsg, "gate$o", gateNum);
}
