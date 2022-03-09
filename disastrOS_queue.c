#include <ucontext.h>

#include "disastrOS_queue.h"
#include "disastrOS.h"
#include "linked_list.h"
#include "disastrOS_globals.h"
#include "disastrOS_wake_queue.h"


int dmq_close(int fd){
    Queue* q = disastrOS_queue_by_fd(fd);
    if (q == 0) return INVALID_FD;
    ListItem** queue_entries = disastrOS_queue_entries(fd);
    // remove current pid from every section they could ever be
    List_detach(&q->readers, queue_entries[0]);
    List_detach(&q->writers, queue_entries[1]);
    List_detach(&q->non_block, queue_entries[2]);

    int retval;
    
    retval = disastrOS_closeResource(fd);
    if(retval) return retval;
    
    q->openings --;

    return 0;
}

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

int dmq_receive(int fd, char* buffer_ptr, int buffer_size){
    Queue* q = disastrOS_queue_by_fd(fd); 
    if (q == 0) return INVALID_FD;
    if (q->msg_size > buffer_size) return BUFFER_TOO_SMALL;

    ListItem** queue_entries = disastrOS_queue_entries(fd);
    
    if (List_find(&q->readers, queue_entries[0]) == 0) return ACTION_NOT_ALLOWED;

    // queue is empty and was started with NON_BLOCK
    if (q->messages.size == 0 && queue_entries[2] != 0)
        return EAGAIN;
    
    int receive_result;
    while ((receive_result = disastrOS_syscall(DSOS_CALL_RECV_MSG, buffer_ptr, fd) == DSOS_NO_MSG_RECEIVED)) disastrOS_preempt(); 

    // wakes up a writer in case the queue, at this point, hasn't been filled again 
    disastrOS_syscall(DSOS_CALL_WAKEUP_QUEUE, &wakeup_flag_condition_recv, q, &(q->writers));

    return 0;
}

int dmq_send(int fd, const char* msg_ptr, int msg_len){
    Queue* q = disastrOS_queue_by_fd(fd); 
    if (q == 0) return INVALID_FD;
    if (q->msg_size < msg_len) return MSG_TOO_BIG;

    ListItem** queue_entries = disastrOS_queue_entries(fd);

    if (List_find(&q->writers, queue_entries[1]) == 0) return ACTION_NOT_ALLOWED;

    // queue is full and was started with NON_BLOCK
    if (q->messages.size == q->max_messages && queue_entries[2] != 0)
        return EAGAIN;
    
    int send_result;
    while ((send_result = disastrOS_syscall(DSOS_CALL_SEND_MSG, msg_ptr, fd)) == DSOS_NO_MSG_SENT) {
        disastrOS_preempt(); 
    }

    // wakes up a reader in case the queue, at this point, hasn't been emptied again
    disastrOS_syscall(DSOS_CALL_WAKEUP_QUEUE, &wakeup_flag_condition_send, q, &(q->readers));
    
    return 0;
}

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

int dmq_unlink(int id){
    return disastrOS_destroyResource(id);
}
