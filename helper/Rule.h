/*
 * Rule.h
 *
 *  Created on: Nov 26, 2019
 *      Author: Kadir Yanık
 */

#ifndef HELPER_RULE_H_
#define HELPER_RULE_H_

#include <stdio.h>
#include <stdint.h>

#define RULE_NO_MATCH 0xFF

class Rule {
private:
    uint8_t source;
    uint8_t destination;
    uint8_t port;
    uint8_t nextDestination;
public:
    Rule();
    virtual ~Rule();
    uint8_t checkRule(uint8_t, uint8_t, uint8_t);
    void setRule(uint8_t, uint8_t, uint8_t, uint8_t);
};

#endif /* HELPER_RULE_H_ */
