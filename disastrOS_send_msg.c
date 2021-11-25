
#include "disastrOS_constants.h"
#include "disastrOS_syscalls.h"

void internal_sendMessage(){
    int fd = running->syscall_args[1];

    Queue* q = disastrOS_queue_by_fd(fd);
    ListItem** queue_entries = disastrOS_queue_entries(fd);
    
    if (q->messages.size < q->max_messages){
        const char* msg = running->syscall_args[0];
        MessageString m_buffer = MsgString_alloc(); 
        Message* m = Message_alloc(m_buffer,q->msg_size);
        for (int i = 0; i < m->len; i++){
            *(m->message + i) = *(msg + i);
            if (*(msg + i) == '\0')
                break;
        }
        List_insert(&q->messages,q->messages.last, m);
        running->syscall_retvalue = 0;
        return;
    }


    
    if (ready_list.first == 0){
        disastrOS_debug("Fatal Error: no process available\n");
        disastrOS_shutdown();
    }

    ((QueueUser*)queue_entries[1])->status = Waiting;
    
    // set process status to wait, until a reader wakes up this process 
    running->status=Waiting;
    List_insert(&waiting_list, waiting_list.last, (ListItem*) running);
    running->syscall_retvalue = DSOS_NO_MSG_SENT; 
    

    
}