#pragma once
#include "linked_list.h"
#include "disastrOS_descriptor.h"

// dmq as for disastrOs message queue

int dmq_close(int fd);
int dmq_getattr(int fd, int attribute_constant);
int dmq_open(int resource_id, int mode);
int dmq_receive(int fd, char* buffer_ptr, int buffer_size);
int dmq_send(int fd, const char* msg_ptr, int msg_len);
int dmq_setattr(int fd, int attribute_constant, int new_val);
int dmq_unlink(int id);