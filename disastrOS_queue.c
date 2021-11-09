#include "disastrOS_queue.h"
#include "disastrOS.h"
#include "linked_list.h"

short unlink_asked = 0;

// TODO: review
int dmq_close(int fd){
    Queue* q = disastrOS_queue_by_fd(fd);
    ListItem** queue_entries = disastrOS_queue_entries(fd);
    // remove current pid from every section they could ever be
    List_detach(&q->readers, queue_entries[0]);
    List_detach(&q->writers, queue_entries[1]);
    List_detach(&q->non_block, queue_entries[2]);
    
    q->openings --;
    if (unlink_asked && q->openings == 0){
        dmq_unlink(fd);
    }
    return 0;
}

// TODO: review
int dmq_getattr(int fd, int attribute_constant){
    Queue* q = disastrOS_queue_by_fd(fd); 
    switch (attribute_constant)
    {
    case ATT_QUEUE_CURRENT_MESSAGES:
        return q->messages.size;
        break;

    case ATT_QUEUE_MAX_MESSAGES:
        return q->max_messages;
        break;
    
    case ATT_QUEUE_MESSAGE_SIZE:
        return q->msg_size;
        break;

    default:
        //throw error or something 
        return -1;
        break;
    }
}

// TODO: start
int dmq_notify();

// TODO: complete 
int dmq_open(int resource_id, int mode){
    return disastrOS_openQueue(resource_id, mode);
}

// TODO: complete
int dmq_receive(int fd, char* buffer_ptr, int buffer_size){
    // check that buffer_size is >= than q->msg_size
    Queue* q = disastrOS_queue_by_fd(fd); 

    Message* message = (Message*) List_pop(&q->messages);

    if (message == 0){
        // TODO handle process block or error in case queue was non-block
        return -1;
    }

    for (int i = 0; i < message->len; i++){
        *(buffer_ptr + i) = *((char*)(message->message + i));
    }

    return 0;
}

// TODO: complete
int dmq_send(int fd, const char* msg_ptr, int msg_len){
    Queue* q = disastrOS_queue_by_fd(fd); 
    ListItem** queue_entries = disastrOS_queue_entries(fd);

    // TODO check that msg_len is <= than q->msg_size

    if (List_find(&q->writers, queue_entries[1]) == 0){
        // TODO throw error: process is not allowed to write
        return;
    }
    if (q->messages.size < q->max_messages){
        Message* m = Message_alloc(msg_ptr,msg_len);
        List_insert(&q->messages,q->messages.last, m);
    }else{
        // TODO handle case list is full (put current sender in wait, unless it's nonblock,
        // in that case, throw an error)
    }
    // TODO rest of interaction with other processes, like setting any processes that was waiting for a message
    // in ready list
    return 0;
}

// TODO: review
int dmq_setattr(int fd, int attribute_constant, void* new_val){
    Queue* q = disastrOS_queue_by_fd(fd); 
    switch (attribute_constant)
    {
    case ATT_QUEUE_CURRENT_MESSAGES:
        // throw error, attribute cannot be set
        break;

    case ATT_QUEUE_MAX_MESSAGES:
        q->max_messages = *((int*)new_val);
        break;

    case ATT_QUEUE_MESSAGE_SIZE:
        q->msg_size = *((int*)new_val);
        break;
    
    default:
        //throw error or something 
        return -1;
        break;
    }
    return 0;
}

// TODO: complete
int dmq_unlink(int fd){
    Queue* q = disastrOS_queue_by_fd(fd);
    if (q->openings > 0){
        unlink_asked = 1;
    }
    if (q->openings == 0){
        disastrOS_closeResource(fd);
        // or disastrOS_destroyResource(fd);
    }
    return 0;

}