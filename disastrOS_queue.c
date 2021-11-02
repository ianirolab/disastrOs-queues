#include "disastrOS_queue.h"
#include "disastrOS.h"
#include "linked_list.h"

// TODO: start
int dmq_close();

// TODO: review
int dmq_getattr(int fd, int attribute_constant){
    Queue* q = disastrOS_queue_by_fd(fd); 
    switch (attribute_constant)
    {
    case QUEUE_CURRENT_MESSAGES:
        return q->current_messages;
        break;

    case QUEUE_MAXIMUM_MESSAGES:
        return q->max_messages;
        break;
    
    default:
        //throw error or something 
        break;
    }
}

// TODO: start
int dmq_notify();

// TODO: complete 
int dmq_open(int resource_id, int mode){
    return disastrOS_openQueue(resource_id, mode);
}

// TODO: start
int dmq_receive();

// TODO: complete
int dmq_send(int fd, const char* msg_ptr, int msg_len){
    Queue* q = disastrOS_queue_by_fd(fd); 
    if (List_find(&q->writers, disastrOS_getpid()) == 0){
        // TODO throw error: process is not allowed to write
        return;
    }
    if (q->current_messages < q->max_messages){
        List_insert(&q->messages,q->messages.last, msg_ptr);
        q->current_messages ++;
    }else{
        // TODO handle case list is full (put current sender in wait, unless it's nonblock,
        // in that case, throw an error)
    }
    // TODO rest of interaction with other processes, like setting any processes that was waiting for a message
    // in ready list
}

int dmq_setattr(int fd, int attribute_constant, void* new_val){
    Queue* q = disastrOS_queue_by_fd(fd); 
    switch (attribute_constant)
    {
    case QUEUE_CURRENT_MESSAGES:
        // throw error, attribute cannot be set
        break;

    case QUEUE_MAXIMUM_MESSAGES:
        q->max_messages = *((int*)new_val);
        break;
    
    default:
        //throw error or something 
        break;
    }
}

// TODO: start
int dmq_unlink();