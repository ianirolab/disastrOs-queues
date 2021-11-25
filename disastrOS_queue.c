#include <ucontext.h>

#include "disastrOS_queue.h"
#include "disastrOS.h"
#include "linked_list.h"
#include "disastrOS_globals.h"
#include "disastrOS_wake_queue.h"

// TODO: done 100%
int dmq_close(int fd){
    Queue* q = disastrOS_queue_by_fd(fd);
    if (q == 0) return INVALID_FD;
    ListItem** queue_entries = disastrOS_queue_entries(fd);
    // remove current pid from every section they could ever be
    List_detach(&q->readers, queue_entries[0]);
    List_detach(&q->writers, queue_entries[1]);
    List_detach(&q->non_block, queue_entries[2]);

    int retval;
    if (q->unlink_request && q->openings == 0){
        // resource is closed after the unlink: weird but it works, given how
        // destroyResource and closeResource work
        retval = dmq_unlink(fd,0);
        if(retval) return retval;
        retval = disastrOS_closeResource(fd);
        if(retval) return retval;
    }else{
        retval = disastrOS_closeResource(fd);
        if(retval) return retval;
    }
    q->openings --;
    return 0;
}

// TODO: done 100%
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
    
    case ATT_QUEUE_UNLINK_REQUEST:
        return q->unlink_request;
        break;
    
    default:
        return INVALID_ATTRIBUTE;
        break;
    }
}

// TODO: done
int dmq_open(int resource_id, int mode){
    return disastrOS_openQueue(resource_id, mode);
}

// TODO: done 
int dmq_receive(int fd, char* buffer_ptr, int buffer_size){
    Queue* q = disastrOS_queue_by_fd(fd); 
    if (q == 0) return INVALID_FD;
    if (q->msg_size > buffer_size) return BUFFER_TOO_SMALL;

    ListItem** queue_entries = disastrOS_queue_entries(fd);
    
    if (List_find(&q->readers, queue_entries[0]) == 0) return ACTION_NOT_ALLOWED;

    // queue is empty and was started with NON_BLOCK
    if (q->messages.size == 0 && queue_entries[2] != 0)
        return EAGAIN;

    Message* message = Message_alloc(buffer_ptr,q->msg_size);
    
    int receive_result;
    while (receive_result = disastrOS_syscall(DSOS_CALL_RECV_MSG, message, fd) == DSOS_NO_MSG_RECEIVED) disastrOS_preempt(); 

    // wakes up a writer in case the queue, at this point, hasn't been filled again 
    disastrOS_syscall(DSOS_CALL_WAKEUP_QUEUE, &wakeup_flag_condition_recv, q, &(q->writers));

    return 0;
}

// TODO: done
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
    while (send_result = disastrOS_syscall(DSOS_CALL_SEND_MSG, msg_ptr, fd) == DSOS_NO_MSG_SENT) {
        disastrOS_preempt(); 
    }

    // wakes up a reader in case the queue, at this point, hasn't been emptied again
    disastrOS_syscall(DSOS_CALL_WAKEUP_QUEUE, &wakeup_flag_condition_send, q, &(q->readers));
    
    return 0;
}

// TODO: done
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

    // TODO: in case there are already messages in the queue, size can't be modified
    case ATT_QUEUE_MESSAGE_SIZE:
        q->msg_size = new_val;
        break;
    
    case ATT_QUEUE_UNLINK_REQUEST:
        q->unlink_request = new_val;
        break;
    
    default: 
        return INVALID_ATTRIBUTE;
        break;
    }
    return 0;
}

// TODO: done 100%
int dmq_unlink(int fd, int schedule){
    Queue* q = disastrOS_queue_by_fd(fd);
    if (q == 0) return INVALID_FD;
    // if a process demands unlink, unlink will fail, if schedule is set to true, unlink will be executed 
    // automatically when all descriptors are closed
    
    if (schedule && q->openings > 0){
        q->unlink_request = 1;
        return UNLINK_SCHEDULED;
    }

    if (q->openings > 0) return UNLINK_FAILED;

    if (q->openings == 0){
        return disastrOS_destroyResource(q->resource_id);
    }
    return UNEXP_NEGATIVE;
}
