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

using namespace omnetpp;

class Switch : public cSimpleModule
{
  protected:
    virtual void forwardMessage(cMessage *msg);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
  private:
    FlowRules flowRules;
};

Define_Module(Switch);

void Switch::initialize()
{
}

void Switch::handleMessage(cMessage *msg)
{
}

void Switch::forwardMessage(cMessage *msg)
{
}



