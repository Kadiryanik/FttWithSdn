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
#include "GeneralMessage_m.h"

using namespace omnetpp;

/*------------------------------------------------------------------------------*/
#define EXPECTED_GATE_MAX_NUM 10
#define NULL_GATE_VAL 0xFF

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

      // initilize to send data to controller
      char data[] = "HELLO";
      GeneralMessage *gMsg = generateMesagge(GM_TYPE_DATA, 0, 0, data);
      scheduleAt(simTime() + 0.1, gMsg);
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
      EV << "handleMessage: Adv message received from " << gMsg->getSource() << "\n";

      int gate = msg->getArrivalGate()->getIndex();
      gateToDest[gate] = gMsg->getSource();
      sendAdvAck(gate);
    } else if(type == GM_TYPE_ADVERTISE_ACK){
      // set gate-destination relation
      gateToDest[msg->getArrivalGate()->getIndex()] = gMsg->getSource();
    } else if(gMsg->getDestination() == id) { // check is message for us
      if(type == GM_TYPE_DATA){
        EV << "handleMessage: Message " << msg << " received from " << gMsg->getSource() << "\n";
        delete msg;
      } else if(type == GM_TYPE_FLOW_RULE){
        flowRules.addRule(gMsg->getFRuleSource(), gMsg->getFRuleDestination(), gMsg->getFRulePort(), gMsg->getFRuleNextDestination());
      } else{
        EV << "handleMessage: Unknown message type!\n";
      }
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

  // get destination's gate from destination id
  for(i = 0; i < n; i++){
    if(gateToDest[i] == dest){
      gateNum = i;
      break;
    }
  }

  if(gateNum == NEXT_DESTINATION_NOT_KNOWN || gateNum >= n){
    gateNum = defaultRoute;
  }

  EV << "forwardMessage: Forwarding message " << (cMessage *)gMsg << " on gate[" << gateNum << "]\n";
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
