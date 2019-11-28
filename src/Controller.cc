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
  virtual void forwardMessage(cMessage *msg);
  virtual void initialize() override;
  virtual void handleMessage(cMessage *msg) override;
  private:

  void sendAdvAck(int gateNum);
  int id;
  Relation* relations;
};

Define_Module(Controller);

/*------------------------------------------------------------------------------*/
Controller::Controller()
{
  relations = NULL;
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

  relations = new Relation(5); // TODO: relations must update when topology changed

  // controller
  relations->addEdge(0, 1);
  relations->addEdge(0, 2);

  // sw 0
  relations->addEdge(1, 3);

  // sw 1
  relations->addEdge(2, 4);

  // sw 2
  relations->addEdge(3, 4);
}

/*------------------------------------------------------------------------------*/
void Controller::handleMessage(cMessage *msg)
{
  GeneralMessage *gMsg = check_and_cast<GeneralMessage *>(msg);
  if(gMsg == NULL){
    EV << "handleMessage: Message cast failed!\n";
    return;
  }

  EV << "handleMessage: Message " << msg << " received from " << gMsg->getSource() << "\n";

  int type = gMsg->getType();
  if(type == GM_TYPE_ADVERTISE){
    sendAdvAck(msg->getArrivalGate()->getIndex());
  }
}

/*------------------------------------------------------------------------------*/
void Controller::forwardMessage(cMessage *msg)
{
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



