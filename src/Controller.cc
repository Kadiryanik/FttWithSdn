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
#include "helper/ElementaryCycle.h"
#include "helper/MessageList.h"

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
  virtual void refreshDisplay() const override;
  virtual void forwardMessage(GeneralMessage *gMsg);
private:
  void forwardMessageToDest(GeneralMessage *gMsg, int destination);
  void forwardMessageAllGate(GeneralMessage *gMsg);
  void sendAdvAck(int gateNum);
  GeneralMessage *generateMesagge(int type, int messageId, int dest, int port, char *data);
  void setFRuleFields(GeneralMessage *gMsg, int src, int dest, int port, int nextDest);
  void setNextHopField(GeneralMessage *gMsg, int arraySize, int *array);
  void setTriggerMessageFields(GeneralMessage *gMsg, TriggerMessage *tm);
  void checkAndSchedule();
  void generateRulesAndSend(int src, int dest, int port);

  cMessage *timerEvent;
  cMessage *timerTrigger;
  int id;
  Relation* relations;
  FlowRules flowRules[TOTAL_SWITCH_NUM + 1]; // +1 for controller
  int gateToDest[EXPECTED_GATE_MAX_NUM];
  MessageList messageList;
  TriggerMessage* currentTriggerMessage;
  int gateInfoCounter;
  ElementaryCycle* currentEC;
  int debugCurrentEC;

  // debug variables
  long numReceived;
  long numSent;
  long numForward;
  long numMessage;
};

Define_Module(Controller);

/*------------------------------------------------------------------------------*/
Controller::Controller()
{
  timerEvent = NULL;
  relations = NULL;

  for(int i = 0; i < EXPECTED_GATE_MAX_NUM; i++){
    gateToDest[i] = NULL_GATE_VAL;
  }

  currentTriggerMessage = new TriggerMessage();
  gateInfoCounter = 0;

  numReceived = 0;
  numSent = 0;
  numForward = 0;
  numMessage = 0;
}

/*------------------------------------------------------------------------------*/
Controller::~Controller()
{
  if(currentTriggerMessage != NULL) delete currentTriggerMessage;

  cancelAndDelete(timerEvent);
  cancelAndDelete(timerTrigger);
  if(relations != NULL) delete relations;
}

/*------------------------------------------------------------------------------*/
void Controller::initialize()
{
  id = par("id");

  relations = new Relation(TOTAL_SWITCH_NUM + 1); // +1 for controller

  WATCH(debugCurrentEC);

  // init pointer, schedule when tm received
  timerTrigger = new cMessage("trigger");

  /* 
   * MESSAGE_ID_0: 6 message per sec
   * MESSAGE_ID_1: 3 message per sec
   * MESSAGE_ID_2: 1 message per sec
   */
  ElementaryCycle* EC1 = new ElementaryCycle();
  EC1->triggerMessage.add(MESSAGE_ID_0, EC_EACH_SLOT_LEN_IN_SEC);
  EC1->triggerMessage.add(MESSAGE_ID_1, EC_EACH_SLOT_LEN_IN_SEC * 2);
  EC1->triggerMessage.add(MESSAGE_ID_2, EC_EACH_SLOT_LEN_IN_SEC * 3);
  EC1->triggerMessage.add(MESSAGE_ID_0, EC_EACH_SLOT_LEN_IN_SEC * 4);

  ElementaryCycle* EC2 = new ElementaryCycle();
  EC2->triggerMessage.add(MESSAGE_ID_0, EC_EACH_SLOT_LEN_IN_SEC);
  EC2->triggerMessage.add(MESSAGE_ID_0, EC_EACH_SLOT_LEN_IN_SEC * 2);
  EC2->triggerMessage.add(MESSAGE_ID_1, EC_EACH_SLOT_LEN_IN_SEC * 3);

  ElementaryCycle* EC3 = new ElementaryCycle();
  EC3->triggerMessage.add(MESSAGE_ID_0, EC_EACH_SLOT_LEN_IN_SEC);

  ElementaryCycle* EC4 = new ElementaryCycle();
  EC4->triggerMessage.add(MESSAGE_ID_0, EC_EACH_SLOT_LEN_IN_SEC);
  EC4->triggerMessage.add(MESSAGE_ID_1, EC_EACH_SLOT_LEN_IN_SEC * 2);

  // we start with next pointer of this so use tail instead of head
  this->currentEC = ElementaryCycle::tail;
}

/*------------------------------------------------------------------------------*/
void Controller::handleMessage(cMessage *msg)
{
  numMessage = messageList.getMessageCount();
  refreshDisplay();

  if(msg == timerEvent){
    // TODO: cancel if exist any timer except this

    // set currentEC
    currentEC = currentEC->next;
    debugCurrentEC = currentEC->elementaryCycleId;

    // generate TM message
    GeneralMessage *gMsg = generateMesagge(GM_TYPE_TM, 0, BROADCAST_ID, 0, (char *)"TM");
    setTriggerMessageFields(gMsg, &(currentEC->triggerMessage));

    // send it all gate
    forwardMessageAllGate(gMsg);

    // schedule next EC start point to wake up
    scheduleAt(simTime() + (EC_TOTAL_SLOT_NUM * EC_EACH_SLOT_LEN_IN_SEC), timerEvent);
    checkAndSchedule();
    return; // this is self message so do not continue the function
  } else if(msg == timerTrigger){
    GeneralMessage* message = messageList.get(currentTriggerMessage->getId(currentTriggerMessage->getOffset()));

    int dest = flowRules[CONTROLLER_INDEX].getNextDestination(message->getSource(), message->getDestination(), message->getPort());
    if(dest != DESTINATION_NOT_FOUND){
      forwardMessageToDest(message, dest);
    } else{
      EV << "timerTrigger: DESTINATION_NOT_FOUND!\n"; // didnt expect
    }
    checkAndSchedule();
    return; // this is self message so do not continue the function
  }

  GeneralMessage *gMsg = check_and_cast<GeneralMessage *>(msg);
  if(gMsg == NULL){
    EV << "handleMessage: Message cast failed!\n";
    return;
  }

  int type = gMsg->getType();

  if(gMsg->getSource() != CONTROLLER_ID){
    EV << "handleMessage: Message [" << type << "] received from Switch_" << (getIndexFromId(gMsg->getSource()) - 1) << "\n";
  }

  if(type == GM_TYPE_ADVERTISE){
    int gate = msg->getArrivalGate()->getIndex();
    gateToDest[gate] = gMsg->getSource();
    sendAdvAck(gate);

    delete msg;
#if WITH_ADMISSION_CONTROL
  } else if(type == GM_TYPE_DATA || type == GM_TYPE_ADMISSION){
#else /* WITH_ADMISSION_CONTROL */
  } else if(type == GM_TYPE_DATA){
#endif /* WITH_ADMISSION_CONTROL */
    if(gMsg->getDestination() != CONTROLLER_ID){
      messageList.add(gMsg);
      checkAndSchedule();
    } else{
      numReceived++;
      EV << "handleMessage: DATA[" << gMsg->getMessageId() << "] received from Switch_" << (getIndexFromId(gMsg->getSource()) - 1) << "\n";
      delete msg;
    }

#if 0 // TODO: this area will be usefull in asynchronous traffic
    int dest = flowRules[CONTROLLER_INDEX].getNextDestination(gMsg->getSource(), gMsg->getDestination(), gMsg->getPort());
    if(dest != DESTINATION_NOT_FOUND){
      forwardMessageToDest(gMsg, dest);
    } else{ // we have no rule, genarete rules
      
    }
#endif
  } else if(type == GM_TYPE_FLOW_RULE){
    forwardMessage(gMsg);
  } else if(type == GM_TYPE_GATE_INFO){
    EV << "  GateInfo: Switch_" << (getIndexFromId(gMsg->getSource()) - 1) << ": ";
    unsigned int i, arrSize = gMsg->getGateInfoArraySize();
    for(i = 0; i < arrSize; i++){
      relations->addEdge(getIndexFromId(gMsg->getSource()), getIndexFromId(gMsg->getGateInfo(i)));
      if(i < arrSize - 1){
        EV << gMsg->getGateInfo(i) << ", ";
      }
    }
    EV << gMsg->getGateInfo(arrSize - 1) << "\n";

    gateInfoCounter++; // increment the received GATE_INFO message counter
    if(gateInfoCounter == TOTAL_SWITCH_NUM){ // All GATE_INFO message received from switches
#if MESSAGE_ID_0_ENABLED
      generateRulesAndSend(MESSAGE_ID_0_SRC, MESSAGE_ID_0_DEST, MESSAGE_ID_0_PORT);
#endif /* MESSAGE_ID_0_ENABLED */
#if MESSAGE_ID_1_ENABLED
      generateRulesAndSend(MESSAGE_ID_1_SRC, MESSAGE_ID_1_DEST, MESSAGE_ID_1_PORT);
#endif /* MESSAGE_ID_1_ENABLED */
#if MESSAGE_ID_2_ENABLED
      generateRulesAndSend(MESSAGE_ID_2_SRC, MESSAGE_ID_2_DEST, MESSAGE_ID_2_PORT);
#endif /* MESSAGE_ID_2_ENABLED */
#if MESSAGE_ID_3_ENABLED
      generateRulesAndSend(MESSAGE_ID_3_SRC, MESSAGE_ID_3_DEST, MESSAGE_ID_3_PORT);
#endif /* MESSAGE_ID_3_ENABLED */
#if MESSAGE_ID_4_ENABLED
      generateRulesAndSend(MESSAGE_ID_4_SRC, MESSAGE_ID_4_DEST, MESSAGE_ID_4_PORT);
#endif /* MESSAGE_ID_4_ENABLED */
#if MESSAGE_ID_5_ENABLED
      generateRulesAndSend(MESSAGE_ID_5_SRC, MESSAGE_ID_5_DEST, MESSAGE_ID_5_PORT);
#endif /* MESSAGE_ID_5_ENABLED */

      // set timer to start EC frames
      timerEvent = new cMessage("event");
      scheduleAt(simTime() + 0.01, timerEvent);
    }

    delete msg;
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

  if(gMsg->getSource() == CONTROLLER_ID){
    numSent++;
  } else{
    numForward++;
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

  if(gMsg->getSource() == CONTROLLER_ID){
    numSent++;
  } else{
    numForward++;
  }

  EV << "forwardMessageToDest: Forwarding message on gate[" << gateNum << "] to Switch_" << (getIndexFromId(destination) - 1) << "\n";
  // $o and $i suffix is used to identify the input/output part of a two way gate
  send(gMsg, "gate$o", gateNum);
}

/*------------------------------------------------------------------------------*/
void Controller::forwardMessageAllGate(GeneralMessage *gMsg)
{
  int i, n = gateSize("gate");

  EV << "forwardMessageAllGate: Forwarding message!\n";
  // $o and $i suffix is used to identify the input/output part of a two way gate
  for(i = 0; i < n; i++){
    GeneralMessage *copy = gMsg->dup();
    send(copy, "gate$o", i);
  }
  delete gMsg;
}

/*------------------------------------------------------------------------------*/
void Controller::sendAdvAck(int gateNum)
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
GeneralMessage* Controller::generateMesagge(int type, int messageId, int dest, int port, char *data)
{
  GeneralMessage *gMsg = new GeneralMessage(data);
  if(gMsg == NULL){
    return NULL;
  }

  gMsg->setType(type);
  gMsg->setMessageId(type);
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

/*------------------------------------------------------------------------------*/
void Controller::setTriggerMessageFields(GeneralMessage *gMsg, TriggerMessage *tm)
{
  int usedNum = tm->getUsedNum();
  int ids[usedNum];
  double times[usedNum];

  currentTriggerMessage->clear();

  for(int i = 0; i < usedNum; i++){
    ids[i] = tm->getId(i);
    times[i] = simTime().dbl() + CHANNEL_DELAYS + tm->getTime(i); // add alfa (CHANNEL_DELAYS)

    currentTriggerMessage->add(ids[i], times[i]);
  }

  gMsg->setTriggerMessageIdArraySize(usedNum);
  gMsg->setTriggerMessageTimeArraySize(usedNum);

  for(int i = 0; i < usedNum; i++){
    gMsg->setTriggerMessageId(i, ids[i]);
    gMsg->setTriggerMessageTime(i, times[i]);
  }
}

/*------------------------------------------------------------------------------*/
void Controller::checkAndSchedule(){
  if(timerTrigger->isScheduled()){ // if not scheduled
    return; // timerTrigger already scheduled
  }

  int i;
  for(i = 0; i < currentTriggerMessage->getUsedNum(); i++){
    if(simTime().dbl() + CHANNEL_DELAYS < currentTriggerMessage->getTime(i)){
      break; // not passed first slot found
    }
  }

  for(; i < currentTriggerMessage->getUsedNum(); i++){
    if(messageList.check(currentTriggerMessage->getId(i)) == ML_MSG_EXIST){
      EV << "[" << currentTriggerMessage->getId(i) << "->" << currentTriggerMessage->getTime(i) << "] scheduled!\n";
      currentTriggerMessage->setOffset(i); // point next one
      scheduleAt(currentTriggerMessage->getTime(i), timerTrigger);
      return;
    }
  }
}

void Controller::generateRulesAndSend(int src, int dest, int port){
  vector<int> path = relations->getShortestDistance(getIndexFromId(src), getIndexFromId(dest));

  EV << "generateRulesAndSend: shortest path: ";
  printIndexInHR(getIndexFromId(src), 0);
  EV << " -> ";
  printIndexInHR(getIndexFromId(dest), 0);
  EV << ":";
  for(int i = path.size() - 1; i >= 0; i--){
    EV << " ";
    printIndexInHR(path[i], 0);
  }
  EV << "\n";



  for(int i = path.size() - 1; i > 0; i--){ // do not checking path[0] because of that the last path is the destination, do not send it any rule
      if(path[i] == CONTROLLER_INDEX){
        flowRules[CONTROLLER_INDEX].addRule(src, dest, port, getIdFromIndex(path[i - 1]));
      } else{
        GeneralMessage *fRuleMsg = generateMesagge(GM_TYPE_FLOW_RULE, 0, getIdFromIndex(path[i]), port, (char *)"FLOW-RULE");
        setFRuleFields(fRuleMsg, src, dest, port, getIdFromIndex(path[i - 1]));
#if !CONTROLLER_CONNECTED_ALL_DIRECTLY // TODO: disable when controller not connected directly all node
        vector<int> nextHops = relations->getShortestDistance(CONTROLLER_INDEX, path[i]); // us to message owner path
        
        int arraySize = nextHops.size() - 1;
        if(arraySize > 0){
          int array[arraySize];
          int offset = 0;

          EV << "  NextHops: ";
          for(int j = arraySize - 1; j >= 0; j--){ // arraySize - 1 mean pass the us
            printIndexInHR(nextHops[j], 0);
            array[offset++] = getIdFromIndex(nextHops[j]);
          }
          EV << "\n";

          setNextHopField(fRuleMsg, arraySize, array);
        } else{
          EV << "  NextHops: null! Controller to Switch_" << (path[i] - 1) << "\n";
          setNextHopField(fRuleMsg, 0, NULL);
        }
#endif
        forwardMessage(fRuleMsg);
      }
  }
}

/*------------------------------------------------------------------------------*/
void Controller::refreshDisplay() const
{
    char buf[50];
    sprintf(buf, "R:%ld S:%ld F:%ld L:%ld", numReceived, numSent, numForward, numMessage);
    getDisplayString().setTagArg("t", 0, buf);
}
