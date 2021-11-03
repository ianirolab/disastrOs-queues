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
  ListHead messages;
  ListHead readers;
  ListHead writers;
  ListHead non_block;
  int max_messages;
  int msg_size;
  int current_messages;
  int openings;
} Queue;

typedef ListHead ResourceList;

// Resource section
void Resource_init();
Resource* Resource_alloc(int id, int type);
int Resource_free(Resource* resource);

// Queue section
void Queue_init();
Queue* Queue_alloc();
void Queue_add_pid(Queue* q, int pid, int mode);
int Queue_free(Queue* queue);

// Debug
Resource* ResourceList_byId(ResourceList* l, int id);
void ResourceList_print(ListHead* l);
