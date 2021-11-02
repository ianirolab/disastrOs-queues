#pragma once
#include "linked_list.h"
#include "disastrOS_descriptor.h"

// dmq as for disastrOs message queue

int dmq_close();
int dmq_getattr();
int dmq_notify();
int dmq_open(int resource_id, int mode);
int dmq_receive();
int dmq_send(int descriptor, const char* msg_ptr, int msg_len);
int dmq_setattr();
int dmq_unlink();