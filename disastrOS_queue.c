#include <ucontext.h>

#include "disastrOS_queue.h"
#include "disastrOS.h"
#include "linked_list.h"
#include "disastrOS_globals.h"
#include "disastrOS_wake_queue.h"

// close a queue for a process, meaning that the queue will not be deleted
// (even if all processes close the queue), but the said process will have
// to open it again in order to interact with it
int dmq_close(int fd){
    Queue* q = disastrOS_queue_by_fd(fd);
    if (q == 0) return INVALID_FD;

    ListItem** queue_entries = disastrOS_queue_entries(fd);
    // remove current pid from every section they could ever be
    QueueUser_free((QueueUser*)List_detach(&q->readers, queue_entries[0]));
    QueueUser_free((QueueUser*)List_detach(&q->writers, queue_entries[1]));
    QueueUser_free((QueueUser*)List_detach(&q->non_block, queue_entries[2]));

    int retval;
    
    // close the main resource
    retval = disastrOS_closeResource(fd);
    if(retval) return retval;
    
    q->openings --;

    return 0;
}

// return various attributes of the queue
int dmq_getattr(int fd, int attribute_constant){
    Queue* q = disastrOS_queue_by_fd(fd); 
    if (q == 0) return INVALID_FD;
    switch (attribute_constant){
    case ATT_QUEUE_CURRENT_MESSAGES:
        return q->messages.size;
        break;

    case ATT_QUEUE_MAX_MESSAGES:
        return q->max_messages;
        break;
    
    case ATT_QUEUE_OPENINGS:
        return q->openings;
        break;

    case ATT_QUEUE_MESSAGE_SIZE:
        return q->msg_size;
        break;
    
    default:
        return INVALID_ATTRIBUTE;
        break;
    }
}

// open a new queue through openResource
int dmq_open(int resource_id, int mode){
    // message_queue resources have type = 2, and creation is either DSOS_CREATE or DSOS_CREATE | DSOS_EXCL
    // depending on the request
    // mode is then used to setup the queue
    int resource_flags = 0;
    if (mode & DSOS_CREAT)
        resource_flags = (mode & DSOS_Q_EXCL) ? DSOS_CREATE | DSOS_EXCL : DSOS_CREATE  ;
        
    int fd = disastrOS_openResource(resource_id,2,resource_flags);
    
    // in case of error, the error is directly returned and the queue is not created
    if (fd < 0) return fd;
    

    return disastrOS_openQueue(resource_id, fd, mode);
    
}

// read one message from the queue and save it in buffer_ptr
// buffer has to be big enough to fit the biggest message possible
int dmq_receive(int fd, char* buffer_ptr, int buffer_size){
    Queue* q = disastrOS_queue_by_fd(fd); 
    if (q == 0) return INVALID_FD;
    if (q->msg_size > buffer_size) return BUFFER_TOO_SMALL;

    ListItem** queue_entries = disastrOS_queue_entries(fd);
    
    // check if the process can read from the queue
    if (List_find(&q->readers, queue_entries[0]) == 0) return ACTION_NOT_ALLOWED;

    // queue is empty and was started with NON_BLOCK
    if (q->messages.size == 0 && queue_entries[2] != 0)
        return EAGAIN;
    
    // keep trying to send the message until successful
    int receive_result;
    while ((receive_result = disastrOS_syscall(DSOS_CALL_RECV_MSG, buffer_ptr, fd) == DSOS_NO_MSG_RECEIVED)) disastrOS_preempt(); 

    // wakes up a writer in case the queue, at this point, hasn't been filled again 
    disastrOS_syscall(DSOS_CALL_WAKEUP_QUEUE, &wakeup_flag_condition_recv, q, &(q->writers));

    return 0;
}

// send msg_ptr to a queue 
int dmq_send(int fd, const char* msg_ptr, int msg_len){
    Queue* q = disastrOS_queue_by_fd(fd); 
    if (q == 0) return INVALID_FD;
    if (q->msg_size < msg_len) return MSG_TOO_BIG;

    ListItem** queue_entries = disastrOS_queue_entries(fd);

    // check if process is a writer
    if (List_find(&q->writers, queue_entries[1]) == 0) return ACTION_NOT_ALLOWED;

    // queue is full and was started with NON_BLOCK
    if (q->messages.size == q->max_messages && queue_entries[2] != 0)
        return EAGAIN;
    
    int send_result;
    // keep trying to send the message until successful
    while ((send_result = disastrOS_syscall(DSOS_CALL_SEND_MSG, msg_ptr, fd)) == DSOS_NO_MSG_SENT) disastrOS_preempt(); 

    // wakes up a reader in case the queue, at this point, hasn't been emptied again
    disastrOS_syscall(DSOS_CALL_WAKEUP_QUEUE, &wakeup_flag_condition_send, q, &(q->readers));
    
    return 0;
}

// set the value of an attribute if possible, or get ATT_NOT_SET
int dmq_setattr(int fd, int attribute_constant, int new_val){
    Queue* q = disastrOS_queue_by_fd(fd); 
    if (q == 0) return INVALID_FD;
    switch (attribute_constant){
    case ATT_QUEUE_CURRENT_MESSAGES:
        return ATT_NOT_SET;
        break;
    
    case ATT_QUEUE_OPENINGS:
        return ATT_NOT_SET;
        break;

    case ATT_QUEUE_MAX_MESSAGES:
        if (new_val > MAX_NUM_MESSAGES_PER_QUEUE) return INVALID_VALUE;
        q->max_messages = new_val;
        break;

    case ATT_QUEUE_MESSAGE_SIZE:
        if (q->messages.size > 0)   return OPERATION_NOT_AVAILABLE;
        q->msg_size = new_val;
        break;
    
    default: 
        return INVALID_ATTRIBUTE;
        break;
    }
    return 0;
}

// unlinks a queue, meaning that all the data it was storing will be deleted
int dmq_unlink(int id){
    // this will take care of any particular case (like if there are still some 
    // descriptors opened)
    return disastrOS_destroyResource(id);
}
