#pragma once
#include "linked_list.h"
#include "disastrOS_pcb.h"


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


typedef char* MessageString;

typedef struct {
  ListItem list;
  MessageString message;
  int len;
} Message;


typedef ListHead MessageList;
typedef ListHead QueueUserList;

typedef struct {
  // list containing all the message currently in the queue
  MessageList messages;

  QueueUserList readers;
  QueueUserList writers;
  QueueUserList non_block;
  
  // maximum number of messages allowed in the queue
  int max_messages;
  // maximum number of characters for a single message
  int msg_size;
  // number of processes that opened the queue
  int openings;
} Queue;

typedef ListHead ResourceList;

// Resource section
void Resource_init();
Resource* Resource_alloc(int id, int type);
int Resource_free(Resource* resource);

// Queue section
void Queue_init();
Queue* Queue_alloc(int resource_id);
int Queue_add_pid(Queue* q, int pid, int mode, ListItem** ds);
int Queue_free(Queue* queue);
void QueueUser_init();
QueueUser* QueueUser_alloc(int pid);
int QueueUser_free(QueueUser* qu);
void Message_init();
Message* Message_alloc(char* message, int message_len);
void MsgString_init();
MessageString MsgString_alloc();
int Message_free(Message* m);

// Debug
Resource* ResourceList_byId(ResourceList* l, int id);
void ResourceList_print(ListHead* l);
void Queue_print();
