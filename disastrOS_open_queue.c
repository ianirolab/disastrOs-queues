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
        // since it's impossible to call syscall in the classic way (in this case disastrOS_closeResource())
        // the syscall is called like a normal function, since all the signals have been deactivated.
        // But since syscalls' arguments are placed in running->syscall_args, we will overwrite those values
        // to simulate a syscall
        // Since internal_destroyResource will be called as well, it's impossible to avoid this overwriting
        running->syscall_args[0] = fd;
        internal_closeResource();
        // in case of error, the retvalue has already the error returned by internal_closeResource
        if (running->syscall_retvalue) return;

        // this way, although it's not really necessary in this case, syscall's args will be 
        // consistent with the original ones
        running->syscall_args[0] = resource_id;
        internal_destroyResource();
        if (running->syscall_retvalue) return;

        running->syscall_retvalue = DSOS_EQUEUEOPEN;
        return;
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
