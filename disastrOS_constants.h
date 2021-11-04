#pragma once

// TODO check that ALL maximums have their own error handling in case they are exceeded
#define MAX_NUM_PROCESSES 1024
#define MAX_NUM_RESOURCES 1024
#define MAX_NUM_QUEUES 1024
#define MAX_NUM_RESOURCES_PER_PROCESS 32
#define MAX_NUM_DESCRIPTORS_PER_PROCESS 32
#define MAX_NUM_MESSAGES_PER_QUEUE 32


#define STACK_SIZE        16384
// signals
#define MAX_SIGNALS 32
#define DSOS_SIGCHLD 0x1
#define DSOS_SIGHUP  0x2

// errors
#define DSOS_ESYSCALL_ARGUMENT_OUT_OF_BOUNDS -1
#define DSOS_ESYSCALL_NOT_IMPLEMENTED -2
#define DSOS_ESYSCALL_OUT_OF_RANGE -3
#define DSOS_EFORK  -4
#define DSOS_EWAIT  -5
#define DSOS_ESPAWN  -6
#define DSOS_ESLEEP  -7
#define DSOS_ERESOURCECREATE -8
#define DSOS_ERESOURCEOPEN -9
#define DSOS_ERESOURCENOEXCL -10
#define DSOS_ERESOURCENOFD -11
#define DSOS_ERESOURCECLOSE -12
#define DSOS_ERESOURCEINUSE -13

// syscall numbers
#define DSOS_MAX_SYSCALLS 32
#define DSOS_MAX_SYSCALLS_ARGS 8
#define DSOS_CALL_PREEMPT   1
#define DSOS_CALL_FORK      2
#define DSOS_CALL_WAIT      3
#define DSOS_CALL_EXIT      4
#define DSOS_CALL_SPAWN     5
#define DSOS_CALL_SLEEP     6
#define DSOS_CALL_OPEN_RESOURCE 7
#define DSOS_CALL_CLOSE_RESOURCE 8
#define DSOS_CALL_DESTROY_RESOURCE 9
#define DSOS_CALL_SHUTDOWN  10

//resources
#define DSOS_CREATE 0x1
#define DSOS_READ 0x2
#define DSOS_WRITE 0x3
#define DSOS_EXCL 0x4

// scheduling
#define ALPHA 0.5f
#define INTERVAL 100 // milliseconds for timer tick


// queues
#define DSOS_RDONLY 00000001
#define DSOS_WRONLY 00000002
#define DSOS_RDWR   00000003
#define DSOS_CREAT  00000010
#define DSOS_Q_EXCL   00000020
#define DSOS_NONBLOCK 00000100
#define DEFAULT_MAX_MESSAGES 1024
#define DEFAULT_MESSAGE_SIZE 4
// queue attributes constants
#define ATT_QUEUE_CURRENT_MESSAGES 0
#define ATT_QUEUE_MAX_MESSAGES 1
#define ATT_QUEUE_MESSAGE_SIZE 2
// queue debug
#define PRINT_QUEUE_MESSAGES 1
#define PRINT_QUEUE_READERS 1
#define PRINT_QUEUE_WRITERS 1
#define PRINT_QUEUE_NONBLOCK 1


