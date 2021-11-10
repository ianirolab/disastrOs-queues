#pragma once
#include "linked_list.h"
#include "disastrOS_pcb.h"

// TODO do all the frees


// structs
typedef struct {
  ListItem list;
  int id;
  int type;
  ListHead descriptors_ptrs;
  void* value;
} Resource;


typedef struct {
  ListItem list;
  int pid;
  // only Waiting and Running from ProcessStatus are used,
  // waiting corresponds the the process 'pid' being in actual waiting status
  // while running doesn't exclusively mean that 'pid' is running, but it might also be ready
  // however that's not important to the purpose of status 
  ProcessStatus status;
} QueueUser;

typedef struct {
  ListItem list;
  char* message;
  int len;
} Message;

typedef struct{
  ListItem list;
  Message* message;
} MessagePtr;

typedef ListHead MessageList;
typedef ListHead QueueUserList;

typedef struct {
  MessageList messages;
  QueueUserList readers;
  QueueUserList writers;
  QueueUserList non_block;
  int max_messages;
  int msg_size;
  int openings;
  int unlink_request;
} Queue;

typedef ListHead ResourceList;

// Resource section
void Resource_init();
Resource* Resource_alloc(int id, int type);
int Resource_free(Resource* resource);

// Queue section
void Queue_init();
Queue* Queue_alloc();
void Queue_add_pid(Queue* q, int pid, int mode, ListItem** ds);
int Queue_free(Queue* queue);
void QueueUser_init();
QueueUser* QueueUser_alloc(int pid);
void Message_init();
Message* Message_alloc(const char* message, int message_len);

// Debug
Resource* ResourceList_byId(ResourceList* l, int id);
void ResourceList_print(ListHead* l);
void Queue_print();
