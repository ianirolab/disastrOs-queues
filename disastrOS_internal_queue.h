#pragma once
#include "linked_list.h"

typedef struct {
  ListHead messages;
  ListHead readers;
  ListHead writers;
  ListHead non_block;
} Queue;


void Queue_init();

Queue* Queue_alloc();
void Queue_add_pid(Queue* q, int pid, int mode);
int Queueu_free(Queue* queue);


// typedef ListHead QueueList;

// Queue* QueueList_byId(QueueList* l, int id);

// void QueueList_print(ListHead* l);