/*
 *  Copyright (c) 2010 Stefano Traverso
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */

#ifndef TRANSACTION_H
#define TRANSACTION_H

/* timeout of the offers thread. If it is not updated, it is deleted */
#define TRANS_ID_MAX_LIFETIME 1.0

struct nodeID;

// register the moment when a transaction is started
// return a  new transaction id
uint16_t transaction_create(struct nodeID *id);

// Add the moment I received a positive select in a list
// return true if a valid trans_id is found
bool transaction_reg_accept(uint16_t trans_id, struct nodeID *id);

// Used to get the time elapsed from the moment I get a positive select to the moment i get the ACK
// related to the same chunk
// it return -1.0 in case no trans_id is found
double transaction_remove(uint16_t trans_id);

// Check the service times list to find elements over the timeout
void check_neighbor_status_list();

#endif // TRANSACTION_H
