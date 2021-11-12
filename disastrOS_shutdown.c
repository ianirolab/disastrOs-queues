#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "disastrOS.h"
#include "disastrOS_syscalls.h"


void internal_shutdown(){
  // resource cleanup
  int resource_n = resources_list.size;
  for (int i = 0; i < resource_n; i++){
    Resource* res=(Resource*) List_detach(&resources_list, (ListItem*) resources_list.first);
    assert(res);
    Resource_free(res);
  }
  shutdown_now=1;
  setcontext(&main_context);
}
