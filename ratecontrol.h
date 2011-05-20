/*
 *  Copyright (c) 2011 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */

#ifndef RATECONTROL_H
#define RATECONTROL_H

#include <stdbool.h>

void rc_reg_accept(uint16_t transid, int accepted);
void rc_reg_ack(uint16_t transid);

#endif //RATECONTROL_H
