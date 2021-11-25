#include "disastrOS_wake_queue.h"

void internal_wakeUpQueue(){
    // there could be writers waiting to send message. Since one message has to be read to get
    // to this point, 
    
    int (*wakeup_check)(Queue*) = (running->syscall_args[0]); 
    int wakeup_flag = wakeup_check(running->syscall_args[1]);
    QueueUserList* queue_users = (QueueUserList*) running->syscall_args[2];

    if (wakeup_flag){
        ListItem* queue_usr = queue_users->first;
        int queue_usr_pid = -1;
        while(queue_usr){
            if (((QueueUser*)queue_usr)->status == Waiting){
                queue_usr_pid = ((QueueUser*)queue_usr)->pid;
                ListItem* aux = waiting_list.first;
                while(aux){
                    if (((PCB*)aux)->pid == queue_usr_pid){
                        ((QueueUser*)queue_usr)->status = Running;
                        List_detach(&waiting_list,aux);
                        List_insert(&ready_list,ready_list.last,aux);
                        running->syscall_retvalue = 0;
                        return;
                    }
                    aux=aux->next;
                }
            }
            queue_usr=queue_usr->next;
        }
    }

    running->syscall_retvalue = 0;

}

int wakeup_flag_condition_recv(Queue* q){
    return ((q->messages.size < (q->max_messages)) && (q->writers.size > 0));
}
int wakeup_flag_condition_send(Queue* q){
    return ((q->messages.size > 0) && (q->readers.size > 0));
}
