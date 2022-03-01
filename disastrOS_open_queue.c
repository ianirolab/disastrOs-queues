#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"
#include "disastrOS_resource.h"
#include "disastrOS_descriptor.h"


void internal_openQueue(){
  int resource_id = running->syscall_args[0];
  int fd = running->syscall_args[1];
  int mode = running->syscall_args[2];
  
  Resource* res = ResourceList_byId(&resources_list, resource_id);
  
  // if res->value is null, it means that the queue hasn't been initialized yet
  if (res->value == 0){
      res->value = Queue_alloc(res->id);
      // if queue couldn't be opened there was a problem (probably pool allocator was full)
      if (res->value == 0){
      disastrOS_closeResource(fd);
      disastrOS_destroyResource(res->id);
      running->syscall_retvalue = DSOS_EQUEUEOPEN;
      } 
  }
  
  // if the file descriptor corresponds to an existing resource that isn't a queue:
  if (res->type != 2) {
    running->syscall_retvalue = INVALID_FD;
    return;
  }

  Descriptor* ds = DescriptorList_byFd(&running->descriptors,fd);
  // current pid is stored as a reader &/| writer &/| nonblock depending on mode 
  if(Queue_add_pid(res->value,running->pid,mode,ds->rwn)) {
    running->syscall_retvalue = LIST_INSERTION_ERROR;
    return;
  }
  running ->syscall_retvalue = fd;
}
