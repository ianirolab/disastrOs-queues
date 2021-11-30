
#include "disastrOS_constants.h"
#include "disastrOS_syscalls.h"

void internal_recvMessage(){
    int fd = running->syscall_args[1];

    Queue* q = disastrOS_queue_by_fd(fd);
    ListItem** queue_entries = disastrOS_queue_entries(fd);
    Message* message = (Message*) List_pop(&q->messages);

    // barbarically copy the message
    Message* msg = running->syscall_args[0];
    if (msg == 0) {
        printf("Error loading the message\n");
        running->syscall_retvalue = DSOS_NO_MSG_RECEIVED;
        return;
    }
    if (message != 0){
        for (int i = 0; i < message->len; i++){
            *(msg->message + i) = *(message->message + i);
            if (message->message + i == '\0')   break;
        }
        Message_free(message);
        running->syscall_retvalue = 0;
        return;
    }
    
    // although this is a syscall (so SIGALARM is ignored), it might still be that this process, after it passed from
    // waiting to ready, will go running after another reader, thus finding the queue empty once again
    if (ready_list.first == 0){
        disastrOS_debug("Fatal Error: no process available\n");
        disastrOS_shutdown();
    }

    ((QueueUser*)queue_entries[0])->status = Waiting;

    // set process status to wait, until a writer wakes up this process 
    running->status=Waiting;
    List_insert(&waiting_list, waiting_list.last, (ListItem*) running);
    running->syscall_retvalue = DSOS_NO_MSG_RECEIVED; 
}
