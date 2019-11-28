/*
 * Controller.cc
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
class Controller : public cSimpleModule
{
public:
  Controller();
  virtual ~Controller();
protected:
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
  virtual void forwardMessage(GeneralMessage *gMsg);
private:
  void forwardMessageToDest(GeneralMessage *gMsg, int destination);
  void sendAdvAck(int gateNum);
  GeneralMessage *generateMesagge(int type, int dest, int port, char *data);
  void setFRuleFields(GeneralMessage *gMsg, int src, int dest, int port, int nextDest);
  void setNextHopField(GeneralMessage *gMsg, int arraySize, int *array);
  int id;
  Relation* relations;
  FlowRules flowRules[TOTAL_SWITCH_NUM + 1]; // +1 for controller
  int gateToDest[EXPECTED_GATE_MAX_NUM];
};

Define_Module(Controller);

/*------------------------------------------------------------------------------*/
Controller::Controller()
{
  relations = NULL;

  for(int i = 0; i < EXPECTED_GATE_MAX_NUM; i++){
    gateToDest[i] = NULL_GATE_VAL;
  }
}

/*------------------------------------------------------------------------------*/
Controller::~Controller()
{
  if(relations != NULL){
    delete relations;
  }
}

/*------------------------------------------------------------------------------*/
void Controller::initialize()
{
  id = par("id");

  relations = new Relation(TOTAL_SWITCH_NUM + 1); // +1 for controller

  // controller
  relations->addEdge(CONTROLLER_INDEX, SWITCH_0_INDEX);
  relations->addEdge(CONTROLLER_INDEX, SWITCH_1_INDEX);

  // sw 0
  relations->addEdge(SWITCH_0_INDEX, SWITCH_2_INDEX);

  // sw 1
  relations->addEdge(SWITCH_1_INDEX, SWITCH_3_INDEX);

  // sw 2
  relations->addEdge(SWITCH_2_INDEX, SWITCH_3_INDEX);
}

/*------------------------------------------------------------------------------*/
void Controller::handleMessage(cMessage *msg)
{
  GeneralMessage *gMsg = check_and_cast<GeneralMessage *>(msg);
  if(gMsg == NULL){
    EV << "handleMessage: Message cast failed!\n";
    return;
  }

  if(gMsg->getSource() != CONTROLLER_ID){
    EV << "handleMessage: Message received from Switch_" << (getIndexFromId(gMsg->getSource()) - 1) << "\n";
  }

  int type = gMsg->getType();
  if(type == GM_TYPE_ADVERTISE){
    int gate = msg->getArrivalGate()->getIndex();
    gateToDest[gate] = gMsg->getSource();
    sendAdvAck(gate);

    delete msg;
  } else if(type == GM_TYPE_DATA){
    int dest = flowRules[CONTROLLER_INDEX].getNextDestination(gMsg->getSource(), gMsg->getDestination(), gMsg->getPort());
    if(dest != NEXT_DESTINATION_NOT_KNOWN){
      forwardMessageToDest(gMsg, dest);
    } else{ // we have no rule, genarete rules
      vector<int> path = relations->getShortestDistance(getIndexFromId(gMsg->getSource()), getIndexFromId(gMsg->getDestination()));

      EV << "handleMessage: shortest path: ";
      for(int i = path.size() - 1; i >= 0; i--){
        EV << "Switch_" << (path[i] - 1) << " ";
      }
      EV << "\n";

      for(int i = path.size() - 1; i > 0; i--){ // do not checking path[0] because of that the last path is the destination, do not send it any rule
          if(path[i] == CONTROLLER_INDEX){
            flowRules[CONTROLLER_INDEX].addRule(gMsg->getSource(), gMsg->getDestination(), gMsg->getPort(), getIdFromIndex(path[i - 1]));
          } else{
            GeneralMessage *fRuleMsg = generateMesagge(GM_TYPE_FLOW_RULE, getIdFromIndex(path[i]), gMsg->getPort(), NULL); // TODO: data may send
            setFRuleFields(fRuleMsg, gMsg->getSource(), gMsg->getDestination(), gMsg->getPort(), getIdFromIndex(path[i - 1]));

            vector<int> nextHops = relations->getShortestDistance(CONTROLLER_INDEX, path[i]); // us to message owner path
            
            int arraySize = nextHops.size() - 1;
            if(arraySize > 0){
              int array[arraySize];
              int offset = 0;

              EV << "  NextHops: ";
              for(int j = arraySize - 1; j >= 0; j--){ // arraySize - 1 mean pass the us
                EV << "Switch_" << (nextHops[j] - 1) << " ";
                array[offset++] = getIdFromIndex(nextHops[j]);
              }
              EV << "\n";

              setNextHopField(fRuleMsg, arraySize, array);
            } else{
              EV << "  NextHops: null! Controller to Switch_" << (path[i] - 1) << "\n";
              setNextHopField(fRuleMsg, 0, NULL);
            }

            scheduleAt(simTime() + 0.1 + uniform(1, 2), fRuleMsg); // TODO: make the timer value little smart 
          }
      }
    }
  } else if(type == GM_TYPE_FLOW_RULE){
    forwardMessage(gMsg);
  }
}

/*------------------------------------------------------------------------------*/
void Controller::forwardMessage(GeneralMessage *gMsg)
{
  int i, n = gateSize("gate");
  int gateNum = n;
  int dest = gMsg->getDestination();

  if(gMsg->getType() == GM_TYPE_FLOW_RULE){
    unsigned int arrSize = gMsg->getNextHopArraySize();

    if(arrSize > 0){
      // manipulate the dest variable for forwarding GM_TYPE_FLOW_RULE messages
      dest = gMsg->getNextHop(0); // points to next destination
    } // else actually didin't expect
  }

  // get destination's gate from destination id
  for(i = 0; i < n; i++){
    if(gateToDest[i] == dest){
      gateNum = i;
      break;
    }
  }

  if(gateNum >= n){
    if(gMsg->getType() == GM_TYPE_FLOW_RULE){
      EV << "forwardMessage: FATAL! (message Switch_" << (getIndexFromId(gMsg->getDestination()) - 1) << "  nextHop is not connected to our gates for " << (getIndexFromId(dest) - 1) << " )\n";
      return;
    }
  }

  EV << "forwardMessage: Forwarding message on gate[" << gateNum << "]\n";
  // $o and $i suffix is used to identify the input/output part of a two way gate
  send(gMsg, "gate$o", gateNum);
}

/*------------------------------------------------------------------------------*/
void Controller::forwardMessageToDest(GeneralMessage *gMsg, int destination)
{
  int i, n = gateSize("gate");
  int gateNum = n;

  // get destination's gate from destination id
  for(i = 0; i < n; i++){
    if(gateToDest[i] == destination){
      gateNum = i;
      break;
    }
  }

  if(gateNum >= n){
    EV << "forwardMessageToDest: Forwarding message failed!\n";
    return;
  }

  EV << "forwardMessageToDest: Forwarding message on gate[" << gateNum << "] to " << (getIndexFromId(destination) - 1) << "\n";
  // $o and $i suffix is used to identify the input/output part of a two way gate
  send(gMsg, "gate$o", gateNum);
}


/*------------------------------------------------------------------------------*/
void Controller::sendAdvAck(int gateNum)
{
  // Create message object and set header fields.
  GeneralMessage *gMsg = new GeneralMessage(NULL);
  if(gMsg == NULL){
    EV << "sendAdvAck: Message generate failed!\n";
    return;
  }

  gMsg->setType(GM_TYPE_ADVERTISE_ACK);
  gMsg->setSource(id);
  gMsg->setDestination(0);
  gMsg->setPort(0);

  EV << "sendAdvAck: Sending to gate[" << gateNum << "]\n";
  send(gMsg, "gate$o", gateNum);
}

/*------------------------------------------------------------------------------*/
GeneralMessage* Controller::generateMesagge(int type, int dest, int port, char *data)
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
void Controller::setFRuleFields(GeneralMessage *gMsg, int src, int dest, int port, int nextDest)
{
  gMsg->setFRuleSource(src);
  gMsg->setFRuleDestination(dest);
  gMsg->setFRulePort(port);
  gMsg->setFRuleNextDestination(nextDest);
}

/*------------------------------------------------------------------------------*/
void Controller::setNextHopField(GeneralMessage *gMsg, int arraySize, int *array)
{
  gMsg->setNextHopArraySize(arraySize);
  for(int i = 0; i < arraySize; i++){
    gMsg->setNextHop(i, array[i]);
  }
}
