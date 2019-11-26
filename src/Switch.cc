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

class Switch : public cSimpleModule
{
  private:
    uint8_t defaultRoute;
    uint8_t id;
    uint8_t gateToDest[2]; // TODO: how to get from ned file?
  protected:
    virtual void forwardMessage(GeneralMessage *gMsg);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
  private:
    FlowRules flowRules;
};

Define_Module(Switch);

void Switch::initialize()
{
    // parse values for each switch from omnetpp.ini file
    // we actually select the gate[0] to shortest path to controller for each switch
    defaultRoute = par("defaultRoute");
    // get id, too
    id = par("id");
    
    // TODO: move this rubbish to ned file or something else
    if(id == SWITCH_0_ID){
        gateToDest[0] = CONTROLLER_ID;
        gateToDest[1] = SWITCH_2_ID;
    } else if(id == SWITCH_1_ID){
        gateToDest[0] = CONTROLLER_ID;
        gateToDest[1] = SWITCH_3_ID;
    } else if(id == SWITCH_2_ID){
        gateToDest[0] = SWITCH_0_ID;
        gateToDest[1] = SWITCH_3_ID;
    } else if(id == SWITCH_3_ID){
        gateToDest[0] = SWITCH_1_ID;
        gateToDest[1] = SWITCH_2_ID;
    } else{
        // TODO: didn't expect
        EV << "Unknown ID!\n";
    }
    
    if(id != 0) {
        char msgname[20];
        sprintf(msgname, "HELLO");

        // Create message object and set header fields.
        GeneralMessage *gMsg = new GeneralMessage(msgname);
        gMsg->setType(GM_TYPE_DATA);
        gMsg->setSource(id);
        gMsg->setDestination(0);
        gMsg->setPort(0);
        scheduleAt(0.0, gMsg);
    }
}

void Switch::handleMessage(cMessage *msg)
{
    GeneralMessage *gMsg = check_and_cast<GeneralMessage *>(msg);

    if(gMsg == NULL){
        EV << "Message cast failed!\n";
        return;
    }

    if(gMsg->getDestination() == id) { // message for us
        // Message arrived.
        EV << "Message " << msg << " arrived.\n";
        delete msg;
    } else {
        // We need to forward the message.
        forwardMessage(gMsg);
    }
}

void Switch::forwardMessage(GeneralMessage *gMsg)
{
    int i, n = gateSize("gate");
    int gateNum = n;
    uint8_t dest = flowRules.getNextDestination(gMsg->getSource(), gMsg->getDestination(), gMsg->getPort());

    // get destination gate from destination id
    for(i = 0; i < n; i++){
        if(gateToDest[i] == dest){
            gateNum = i;
            break;
        }
    }

    if(gateNum == NEXT_DESTINATION_NOT_KNOWN || gateNum >= n){
        gateNum = defaultRoute;
    }

    EV << "Forwarding message " << (cMessage *)gMsg << " on gate[" << gateNum << "]\n";
    // $o and $i suffix is used to identify the input/output part of a two way gate
    send(gMsg, "gate$o", gateNum);
}



