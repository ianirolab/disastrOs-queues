#include <ucontext.h>

#include "disastrOS_queue.h"
#include "disastrOS.h"
#include "linked_list.h"
#include "disastrOS_globals.h"

// TODO: done
int dmq_close(int fd){
    Queue* q = disastrOS_queue_by_fd(fd);
    if (q == 0) return INVALID_FD;
    ListItem** queue_entries = disastrOS_queue_entries(fd);
    // remove current pid from every section they could ever be
    List_detach(&q->readers, queue_entries[0]);
    List_detach(&q->writers, queue_entries[1]);
    List_detach(&q->non_block, queue_entries[2]);
    
    q->openings --;
    if (q->unlink_request && q->openings == 0){
        dmq_unlink(fd);
    }
    return 0;
}

// TODO: done
int dmq_getattr(int fd, int attribute_constant){
    Queue* q = disastrOS_queue_by_fd(fd); 
    if (q == 0) return INVALID_FD;
    switch (attribute_constant)
    {
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

// TODO: done (1 TODO)
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
            // TODO: check if comment is true, with no possible errors (for example if a reader/writer tries to access it)
            // message is now retrieved. Message can't be null, since the first reader process is taken away from waiting_list
            // only when a message appears
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

// TODO: complete
int dmq_unlink(int fd){
    Queue* q = disastrOS_queue_by_fd(fd);
    if (q->openings > 0){
        q->unlink_request = 1;
    }
    if (q->openings == 0){
        disastrOS_closeResource(fd);
        // or disastrOS_destroyResource(fd);
    }
    return 0;

}