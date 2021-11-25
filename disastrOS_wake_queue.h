#include "disastrOS_constants.h"
#include "disastrOS_syscalls.h"

// internal_wakeUpQueue is already included through disastrOS_syscalls.h


int wakeup_flag_condition_recv(Queue* q);
int wakeup_flag_condition_send(Queue* q);
