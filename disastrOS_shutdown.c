#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"

#define SHUTDOWN_WAITING_CYCLES 5

void internal_shutdown(){
  // since shutdown is a complex operation, it is useful to avoid
  // that it's called multiple times
  if(shutdown_now == 1){
    running->syscall_retvalue = DSOS_SHUTTING;
    return;
  }
  shutdown_now=1;

  ucontext_t graveyard_context = *((ucontext_t*)running->syscall_args[0]);

  // resource cleanup
  int resource_n = resources_list.size;
  for (int i = 0; i < resource_n; i++){
    Resource* res = (Resource*) List_pop(&resources_list);
    int pointers_n = res->descriptors_ptrs.size;
    for (int j = 0; j < pointers_n; j++){
      DescriptorPtr* desptr = (DescriptorPtr*) List_pop(&res->descriptors_ptrs);
      assert(desptr);
      
      Descriptor_free(desptr->descriptor);
      DescriptorPtr_free(desptr);
    }
    Resource_free(res);
  }

  int processes_n = waiting_list.size;
  for(int i = 0; i < processes_n; i++){
    PCB* p = (PCB*) List_pop(&waiting_list);
    if(p->pid == 0){
      List_insert(&waiting_list, waiting_list.last, (ListItem*)p);
      continue;
    }
    p->cpu_state = graveyard_context;
    List_insert(&ready_list,ready_list.last, (ListItem*)p);
  }

  if(processes_n > 0) {
    PCB* temp = running;
    running->syscall_args[0] = SHUTDOWN_WAITING_CYCLES;
    internal_sleep();
    swapcontext(&temp->cpu_state, &running->cpu_state);
  }

  setcontext(&main_context);
}
