/*
 * ElementaryCycle.cpp
 *
 *  Created on: Dec 11, 2019
 *      Author: Kadir Yanık
 */

#include "ElementaryCycle.h"

/*------------------------------------------------------------------------------*/
ElementaryCycle* ElementaryCycle::head = NULL;
ElementaryCycle* ElementaryCycle::tail = NULL;
int ElementaryCycle::elementaryCycleCount = 0;

/*------------------------------------------------------------------------------*/
ElementaryCycle::ElementaryCycle(){
  if(this->head == NULL){ // initialize 1 node circular list
    head = this;
    tail = this;
    this->next = this;
    elementaryCycleCount = 1;
    this->elementaryCycleId = elementaryCycleCount;
  } else{ // add new node to endof list
    ElementaryCycle* ptr = head;

    // find the last node
    while(ptr->next != head){
      ptr = ptr->next;
    }

    ptr->next = this;  // (Last - 1)->next = (Last)
    tail = this;       // tail = (Last)
    this->next = head; // tail->next = head

    elementaryCycleCount++;
    this->elementaryCycleId = elementaryCycleCount;
  }
}

/*------------------------------------------------------------------------------*/
ElementaryCycle::~ElementaryCycle(){
  ElementaryCycle* ptr = head;
  ElementaryCycle* ptrHolder = NULL;

  while(ptr != NULL){
    ptrHolder = ptr->next;
    delete ptr;

    ptr = ptrHolder;
  }
}