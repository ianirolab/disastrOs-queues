#include <ucontext.h>

#include "disastrOS_queue.h"
#include "disastrOS.h"
#include "linked_list.h"
#include "disastrOS_globals.h"

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

    // there could be writers waiting to send message
    short wakeup_flag = (q->messages.size == q->max_messages) && (q->writers.size > 0);
    
    Message* message = (Message*) List_pop(&q->messages);

    // in case there are no messages
    while (message == 0){
        // there are no messages
        if (queue_entries[2] != 0){
            return EAGAIN;
        }

        // put runnning process in wait, and set it to ready as soon as the queue has free space
        if (ready_list.first){
            ((QueueUser*)queue_entries[0])->status = Waiting;
            PCB* current_process = running;
            running->status=Waiting;
            List_insert(&waiting_list, waiting_list.last, (ListItem*) running);
            
            PCB* next_process=(PCB*) List_detach(&ready_list, ready_list.first);
            next_process->status=Running;
            running=next_process;
            swapcontext(&current_process->cpu_state,&next_process->cpu_state);
            message = (Message*) List_pop(&q->messages);
        }else{
            disastrOS_debug("PREEMPT - %d ->", running->pid);
        }

    }

    // barbarically copy the message
    for (int i = 0; i < message->len; i++){
        *(buffer_ptr + i) = *((char*)(message->message + i));
    }

    // in case there might be a writer waiting to send a message
    if (wakeup_flag){
        ListItem* writer = q->writers.first;
        int writer_pid = -1;
        while(writer){
            if (((QueueUser*)writer)->status == Waiting){
                writer_pid = ((QueueUser*)writer)->pid;
                break;
            }
            writer=writer->next;
        }
        // Although there are writers, it might be that none of them is waiting
        if (writer_pid != -1){
            ListItem* aux = waiting_list.first;
            while(aux){
                if (((PCB*)aux)->pid == writer_pid){
                    ((QueueUser*)writer)->status = Running;
                    List_detach(&waiting_list,aux);
                    List_insert(&ready_list,ready_list.last,aux);
                }
                aux=aux->next;
            }
        }
    }
    return 0;
}

// TODO: done
int dmq_send(int fd, const char* msg_ptr, int msg_len){
    Queue* q = disastrOS_queue_by_fd(fd); 
    if (q == 0) return INVALID_FD;
    if (q->msg_size < msg_len) return MSG_TOO_BIG;

    ListItem** queue_entries = disastrOS_queue_entries(fd);

    if (List_find(&q->writers, queue_entries[1]) == 0) return ACTION_NOT_ALLOWED;

    // there could be readers waiting for a message
    short wakeup_flag = q->messages.size == 0 && q->readers.size > 0;
    
    while (q->messages.size == q->max_messages){
        if (queue_entries[2] != 0){
            // queue was started with NON_BLOCK
            return EAGAIN;
        }
        if (ready_list.first){
            ((QueueUser*)queue_entries[1])->status = Waiting;
            PCB* current_process = running;
            running->status=Waiting;
            List_insert(&waiting_list, waiting_list.last, (ListItem*) running);
            
            PCB* next_process=(PCB*) List_detach(&ready_list, ready_list.first);
            next_process->status=Running;
            running=next_process;
            swapcontext(&current_process->cpu_state,&next_process->cpu_state);
        }

    }

    Message* m = Message_alloc(msg_ptr,msg_len);
    List_insert(&q->messages,q->messages.last, m);

    if (wakeup_flag){
        ListItem* reader = q->readers.first;
        int reader_pid = -1;
        while(reader){
            if (((QueueUser*)reader)->status == Waiting){
                reader_pid = ((QueueUser*)reader)->pid;
                break;
            }
            reader=reader->next;
        }
        // Although there are readers, it might be that none of them is waiting
        if (reader_pid != -1){
            ListItem* aux = waiting_list.first;
            while(aux){
                if (((PCB*)aux)->pid == reader_pid){
                    ((QueueUser*)reader)->status = Running;
                    List_detach(&waiting_list,aux);
                    List_insert(&ready_list,ready_list.last,aux);
                }
                aux=aux->next;
            }
        }
    }

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