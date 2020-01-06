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
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
  virtual void forwardMessage(GeneralMessage *gMsg);
  virtual void refreshDisplay() const override;
  virtual void finish() override;
private:
  GeneralMessage* generateMesagge(int type, int messageId, int dest, int port, char *data);
  void sendAdv(int gateNum);
  void sendAdvAck(int gateNum);
  void setGateInfoFields(GeneralMessage *gMsg, int *gateInfo, int gateNum);
  void setCurrentTriggerMessage(GeneralMessage *gMsg);
  void checkAndSchedule();
  void generateTraffic();
  void handleGeneralMessage(cMessage *msg);
  GeneralMessage* getNextMessage();

  int id;
  int advDone;
  int defaultRoute;
  cMessage *timerEvent;

  // database
  int gateToDest[EXPECTED_GATE_MAX_NUM];
  FlowRules flowRules;
  MessageList messageList;
  MessageList messageListAsync;

  // FTT spesific
  TriggerMessage* currentTriggerMessage;
  cMessage *trafficGenerator;
  cMessage *timerTrigger;
  // used on async traffic
  double beginningOfNextEC;

  // debug variables
  long numReceived;
  long numSent;
  long numForward;
  long numMessage;

  // visualize results
  cOutVector delayVector;
  cHistogram delayStats;
};

Define_Module(Switch);

/*------------------------------------------------------------------------------*/
Switch::Switch(){
  timerEvent = NULL;
  advDone = 0;

  int i;
  for(i = 0; i < EXPECTED_GATE_MAX_NUM; i++){
    gateToDest[i] = NULL_GATE_VAL;
  }

  beginningOfNextEC = 0;
  
  currentTriggerMessage = NULL;

  numReceived = 0;
  numSent = 0;
  numForward = 0;
  numMessage = 0;
}
/*------------------------------------------------------------------------------*/
Switch::~Switch(){
  if(currentTriggerMessage) delete currentTriggerMessage;

  cancelAndDelete(timerEvent);
  cancelAndDelete(trafficGenerator);
  cancelAndDelete(timerTrigger);
}

/*------------------------------------------------------------------------------*/
void Switch::initialize(){
  // parse values for each switch from omnetpp.ini file
  // we actually select the gate[0] to shortest path to controller for each switch
  defaultRoute = par("defaultRoute");

  // get id, too
  id = par("id");

  delayVector.setName("delayVector");
  delayStats.setName("delayStats");

  // init pointer, schedule when neccessary
  trafficGenerator = new cMessage("trafficGenerator");

  // init pointer, schedule when tm received
  timerTrigger = new cMessage("trigger");

  // set timer to start advertisement
  timerEvent = new cMessage("event");
  scheduleAt(simTime() + 0.1, timerEvent);
}

/*------------------------------------------------------------------------------*/
void Switch::handleMessage(cMessage *msg){
  numMessage = messageList.getMessageCount();
  refreshDisplay();

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

    scheduleAt(simTime() + 0.1, trafficGenerator);
  } else if(msg == trafficGenerator){
    generateTraffic();
  } else if(msg == timerTrigger){
    GeneralMessage* message = messageList.get(currentTriggerMessage->getId(currentTriggerMessage->getOffset()));
    //EV << "MessageCount = " << messageList.getMessageCount() << "\n";
    forwardMessage(message);
    checkAndSchedule();
  } else{
    handleGeneralMessage(msg);
  }
}

/*------------------------------------------------------------------------------*/
void Switch::forwardMessage(GeneralMessage *gMsg){
  int i, n = gateSize("gate");
  int gateNum = n;
  int dest = flowRules.getNextDestination(gMsg->getSource(), gMsg->getDestination(), gMsg->getPort());

  if(gMsg->getType() == GM_TYPE_FLOW_RULE){
    int arrSize = (int)(gMsg->getNextHopArraySize());

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
    numForward++;
  } else{
    numSent++;
  }

  EV << "forwardMessage: Forwarding message [" << gMsg->getMessageId() << "] on gate[" << gateNum << "]\n";
  // $o and $i suffix is used to identify the input/output part of a two way gate
  send(gMsg, "gate$o", gateNum);

  refreshDisplay();
}

/*------------------------------------------------------------------------------*/
void Switch::refreshDisplay() const
{
    char buf[50];
    sprintf(buf, "R:%ld S:%ld F:%ld L:%ld", numReceived, numSent, numForward, numMessage);
    getDisplayString().setTagArg("t", 0, buf);
}

void Switch::finish(){
    // This function is called by OMNeT++ at the end of the simulation.
    EV << "Sent:     " << numSent << endl;
    EV << "Received: " << numReceived << endl;
    EV << "delay, min:    " << delayStats.getMin() << endl;
    EV << "delay, max:    " << delayStats.getMax() << endl;
    EV << "delay, mean:   " << delayStats.getMean() << endl;
    EV << "delay, stddev: " << delayStats.getStddev() << endl;

    recordScalar("#sent", numSent);
    recordScalar("#received", numReceived);

    delayStats.recordAs("delay");
}


/*------------------------------------------------------------------------------*/
GeneralMessage* Switch::generateMesagge(int type, int messageId, int dest, int port, char *data){
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
void Switch::sendAdv(int gateNum){
  GeneralMessage *gMsg = generateMesagge(GM_TYPE_ADVERTISE, 0, 0, 0, (char *)"ADV");
  if(gMsg == NULL){
    EV << "sendAdv: Message generate failed!\n";
    return;
  }

  EV << "sendAdv: Sending to gate[" << gateNum << "]\n";
  send(gMsg, "gate$o", gateNum);
}

/*------------------------------------------------------------------------------*/
void Switch::sendAdvAck(int gateNum){
  GeneralMessage *gMsg = generateMesagge(GM_TYPE_ADVERTISE_ACK, 0, 0, 0, (char *)"ADV-ACK");
  if(gMsg == NULL){
    EV << "sendAdvAck: Message generate failed!\n";
    return;
  }

  EV << "sendAdvAck: Sending to gate[" << gateNum << "]\n";
  send(gMsg, "gate$o", gateNum);
}

/*------------------------------------------------------------------------------*/
void Switch::setGateInfoFields(GeneralMessage *gMsg, int *gateInfo, int gateNum){
  gMsg->setGateInfoArraySize(gateNum);
  for(int i = 0; i < gateNum; i++){
    gMsg->setGateInfo(i, gateInfo[i]);
  }
}

/*------------------------------------------------------------------------------*/
void Switch::checkAndSchedule(){
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

/*------------------------------------------------------------------------------*/
void Switch::setCurrentTriggerMessage(GeneralMessage *gMsg){
  if(currentTriggerMessage == NULL){
    currentTriggerMessage = new TriggerMessage();
  } else{
    currentTriggerMessage->clear();
  }

  EV << "setCurrentTriggerMessage:";
  for(unsigned int i = 0; i < gMsg->getTriggerMessageIdArraySize(); i++){
    currentTriggerMessage->add(gMsg->getTriggerMessageId(i), gMsg->getTriggerMessageTime(i));

    EV << " [" << currentTriggerMessage->getId(i) << "->" << currentTriggerMessage->getTime(i) << "]";
  }
  EV << "\n";
}

/*------------------------------------------------------------------------------*/
void Switch::generateTraffic(){
  int reSchedule = 0;
  char data[20];
  GeneralMessage* message;

#if MESSAGE_ID_0_ENABLED
  if(id == MESSAGE_ID_0_SRC){
    static int seqNumMsg01;
    for(int i = 0; i < MESSAGE_ID_0_PER_SEC; i++){
      sprintf(data, "MSG_0 %d", seqNumMsg01++);
      message = generateMesagge(GM_TYPE_DATA, MESSAGE_ID_0, MESSAGE_ID_0_DEST, MESSAGE_ID_0_PORT, data);
      messageList.add(message);
    }
    reSchedule = 1;
  }
#endif /* MESSAGE_ID_0_ENABLED */
#if MESSAGE_ID_1_ENABLED
  if(id == MESSAGE_ID_1_SRC){
    static int seqNumMsg1;
    for(int i = 0; i < MESSAGE_ID_1_PER_SEC; i++){
      sprintf(data, "MSG_1 %d", seqNumMsg1++);
      message = generateMesagge(GM_TYPE_DATA, MESSAGE_ID_1, MESSAGE_ID_1_DEST, MESSAGE_ID_1_PORT, data);
      messageList.add(message);
    }
    reSchedule = 1;
  }
#endif /* MESSAGE_ID_1_ENABLED */
#if MESSAGE_ID_2_ENABLED
  if(id == MESSAGE_ID_2_SRC){
    static int seqNumMsg2;
    for(int i = 0; i < MESSAGE_ID_2_PER_SEC; i++){
      sprintf(data, "MSG_2 %d", seqNumMsg2++);
      message = generateMesagge(GM_TYPE_DATA, MESSAGE_ID_2, MESSAGE_ID_2_DEST, MESSAGE_ID_2_PORT, data);
      messageList.add(message);
    }
    reSchedule = 1;
  }
#endif /* MESSAGE_ID_2_ENABLED */
#if MESSAGE_ID_3_ENABLED
  if(id == MESSAGE_ID_3_SRC){
    static int seqNumMsg02;
    for(int i = 0; i < MESSAGE_ID_3_PER_SEC; i++){
      sprintf(data, "MSG_0 %d", seqNumMsg02++);
      message = generateMesagge(GM_TYPE_DATA, MESSAGE_ID_3, MESSAGE_ID_3_DEST, MESSAGE_ID_3_PORT, data);
      messageList.add(message);
    }
    reSchedule = 1;
  }
#endif /* MESSAGE_ID_3_ENABLED */
#if MESSAGE_ID_4_ENABLED
  if(id == MESSAGE_ID_4_SRC){
    static int seqNumMsg02;
    for(int i = 0; i < MESSAGE_ID_4_PER_SEC; i++){
      sprintf(data, "MSG_0 %d", seqNumMsg02++);
      message = generateMesagge(GM_TYPE_DATA, MESSAGE_ID_4, MESSAGE_ID_4_DEST, MESSAGE_ID_4_PORT, data);
      messageList.add(message);
    }
    reSchedule = 1;
  }
#endif /* MESSAGE_ID_4_ENABLED */
#if MESSAGE_ID_5_ENABLED
  if(id == MESSAGE_ID_5_SRC){
    static int seqNumMsg02;
    for(int i = 0; i < MESSAGE_ID_5_PER_SEC; i++){
      sprintf(data, "MSG_1 %d", seqNumMsg02++);
      message = generateMesagge(GM_TYPE_DATA, MESSAGE_ID_5, MESSAGE_ID_5_DEST, MESSAGE_ID_5_PORT, data);
      messageList.add(message);
    }
    reSchedule = 1;
  }
#endif /* MESSAGE_ID_5_ENABLED */
  if(reSchedule){
    scheduleAt(simTime() + 1, trafficGenerator);
  }
}

/*------------------------------------------------------------------------------*/
void Switch::handleGeneralMessage(cMessage *msg){
  GeneralMessage *gMsg = check_and_cast<GeneralMessage *>(msg);
  if(gMsg == NULL){
    EV << "handleMessage: Message cast failed!\n";
    return;
  }

  int type = gMsg->getType();
  
  // destination area is not valid for advertisement messages so check them first
  if(type == GM_TYPE_ADVERTISE){
    EV << "handleGM: Adv message received from ";
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
      numReceived++;
      EV << "handleGM: Message " << msg << " received from ";
      printIndexInHR(getIndexFromId(gMsg->getSource()), 1);

      EV << "handleGM: ARR " << msg->getArrivalTime() << "-" << msg->getCreationTime() << "\n";
      delayVector.record(msg->getArrivalTime() - msg->getCreationTime());
      delayStats.collect(msg->getArrivalTime() - msg->getCreationTime());
    } else if(type == GM_TYPE_FLOW_RULE){
      EV << "handleGM: FlowRule received from ";
      printIndexInHR(getIndexFromId(gMsg->getSource()), 1);

      flowRules.addRule(gMsg->getFRuleSource(), gMsg->getFRuleDestination(), gMsg->getFRulePort(), gMsg->getFRuleNextDestination());
    } else if(type == GM_TYPE_ASYNC){
      if(simTime().dbl() + CHANNEL_DELAYS < beginningOfNextEC){
        EV << "handleGM: Sending Async message\n";
        GeneralMessage *message = getNextMessage();
        if(message != NULL){
          forwardMessage(message);
        }
      }
    } else{
      EV << "handleGM: Unknown message type " << type << "!\n";
    }

    delete msg; // free received msg
  } else if(type == GM_TYPE_TM){
    setCurrentTriggerMessage(gMsg);

    // set next EC start point
    beginningOfNextEC = simTime().dbl() + (EC_TOTAL_SLOT_NUM * EC_EACH_SLOT_LEN_IN_SEC);

    checkAndSchedule();

    delete msg;
  } else {
    if(currentTriggerMessage == NULL){ // TODO: make this much smarter
      forwardMessage(gMsg);
    } else{
      // add messageList for forwarding
      messageList.add(gMsg);
      checkAndSchedule();
    }
  }
}

/*------------------------------------------------------------------------------*/
GeneralMessage* Switch::getNextMessage(){
  GeneralMessage *msg = messageListAsync.getNext();
  if(msg == NULL){ // Async list empty try sync
    msg = messageList.getNext();
  }

  return msg;
}
