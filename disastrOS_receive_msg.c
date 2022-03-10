#include <stdio.h>
#include "disastrOS_constants.h"
#include "disastrOS_syscalls.h"

void internal_recvMessage(){
    int fd = running->syscall_args[1];

    Queue* q = disastrOS_queue_by_fd(fd);
    ListItem** queue_entries = disastrOS_queue_entries(fd);
    char* msg = (char*) running->syscall_args[0];

    if (msg == 0) {
        printf("Error loading the message buffer\n");
        running->syscall_retvalue = DSOS_NO_MSG_RECEIVED;
        return;
    }

    Message* message = (Message*) List_pop(&q->messages);
    
    if (message != 0){
        // copy the message into the buffer
        for (int i = 0; i < message->len; i++){
            *(msg + i) = *(message->message + i);
            if ( *(message->message + i) == '\0')   break;
        }
        Message_free(message);
        running->syscall_retvalue = 0;
        return;
    }

    // message is null, the process goes into waiting until the queue is non-empty
    ((QueueUser*)queue_entries[0])->status = Waiting;

    running->status=Waiting;
    List_insert(&waiting_list, waiting_list.last, (ListItem*) running);
    running->syscall_retvalue = DSOS_NO_MSG_RECEIVED; 
}
