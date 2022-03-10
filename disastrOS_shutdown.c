#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"

// defines how long will the shutdown wait for all processes to end
// through graveyard_context
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

  int processes_r = ready_list.size;
  
  // all ready processes go to graveyard context, so that they can return
  // to the waiting parent (with the appropriate value)
  // process 0 (init) and 1 (clock) are obviously ignored
  for (int i = 0; i < processes_r; i++){
    PCB* p = (PCB*) List_pop(&ready_list);
    if (p->pid == 0 || p->pid == 1){
      List_insert(&ready_list, ready_list.last, (ListItem*) p);
      continue;
    }
    p->cpu_state = graveyard_context;
  }

  // all waiting processes are set to ready (so that they won't be
  // ignored by the scheduler) and meet the same destiny as ready processes
  // process 0 (init) and 1 (clock) are obviously ignored
  int processes_w = waiting_list.size;
  for(int i = 0; i < processes_w; i++){
    PCB* p = (PCB*) List_pop(&waiting_list);
    if(p->pid == 0 || p->pid == 1){
      List_insert(&waiting_list, waiting_list.last, (ListItem*)p);
      continue;
    }
    p->cpu_state = graveyard_context;
    List_insert(&ready_list,ready_list.last, (ListItem*)p);
  }

  // leave some time to let the scheduler call in the processes
  // that have to exit
  // process 0 (init) and 1 (clock) have been ignored 
  if((processes_r + processes_w - 2) > 0) {
    PCB* temp = running;
    running->syscall_args[0] = SHUTDOWN_WAITING_CYCLES;
    internal_sleep();
    swapcontext(&temp->cpu_state, &running->cpu_state);
  }

  // terminate the shutdown
  setcontext(&main_context);
}
