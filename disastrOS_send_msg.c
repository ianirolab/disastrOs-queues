#include <stdio.h>

#include "disastrOS_constants.h"
#include "disastrOS_syscalls.h"

void internal_sendMessage(){
    int fd = running->syscall_args[1];

    Queue* q = disastrOS_queue_by_fd(fd);
    ListItem** queue_entries = disastrOS_queue_entries(fd);
    
    if (q->messages.size < q->max_messages){
        const char* msg = (const char*) running->syscall_args[0];
        MessageString m_buffer = MsgString_alloc(); 
        Message* m = Message_alloc(m_buffer,q->msg_size);
        if (m == 0 || m_buffer == 0) {
            printf("Message not alloc'd pid: %d\n", running->pid);
            running->syscall_retvalue = DSOS_NO_MSG_SENT;
            return;
        }
        // copy the message into the structure
        for (int i = 0; i < m->len; i++){
            *(m->message + i) = *(msg + i);
            if (*(msg + i) == '\0')
                break;
        }
        // add the Message to the queue
        List_insert(&q->messages,q->messages.last, (ListItem*) m);
        running->syscall_retvalue = 0;
        return;
    }

    // queue is full, go into waiting until the queue has space again
    ((QueueUser*)queue_entries[1])->status = Waiting;
    
    running->status=Waiting;
    List_insert(&waiting_list, waiting_list.last, (ListItem*) running);
    running->syscall_retvalue = DSOS_NO_MSG_SENT; 
    

    
}