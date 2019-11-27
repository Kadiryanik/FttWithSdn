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
#include "GeneralMessage_m.h"

using namespace omnetpp;

/*------------------------------------------------------------------------------*/
class Controller : public cSimpleModule
{
  protected:
    virtual void forwardMessage(cMessage *msg);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
  private:
    void sendAdvAck(int gateNum);
    int id;
};

Define_Module(Controller);

/*------------------------------------------------------------------------------*/
void Controller::initialize()
{
    id = par("id");
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
    // advertise messages destination address not valid, so check firs type
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



