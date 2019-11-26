/*
 * Controller.cc
 *
 *  Created on: Nov 26, 2019
 *      Author: Kadir Yanık
 */

#include <stdio.h>
#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

class Controller : public cSimpleModule
{
  protected:
    virtual void forwardMessage(cMessage *msg);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(Controller);

void Controller::initialize()
{
}

void Controller::handleMessage(cMessage *msg)
{
}

void Controller::forwardMessage(cMessage *msg)
{
}






