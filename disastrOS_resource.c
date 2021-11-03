#include <assert.h>
#include <stdio.h>
#include "disastrOS_resource.h"
#include "disastrOS_descriptor.h"
#include "pool_allocator.h"
#include "linked_list.h"

#define RESOURCE_SIZE sizeof(Resource)
#define RESOURCE_MEMSIZE (sizeof(Resource)+sizeof(int))
#define RESOURCE_BUFFER_SIZE MAX_NUM_RESOURCES*RESOURCE_MEMSIZE

#define QUEUE_SIZE sizeof(Queue)
#define QUEUE_MEMSIZE (sizeof(Queue)+sizeof(int))
#define QUEUE_BUFFER_SIZE MAX_NUM_QUEUES*QUEUE_MEMSIZE
#define QUEUE_MAX_MESSAGES 1024

static char _resources_buffer[RESOURCE_BUFFER_SIZE];
static PoolAllocator _resources_allocator;

static char _queues_buffer[QUEUE_BUFFER_SIZE];
static PoolAllocator _queues_allocator;

// General resource section

void Resource_init(){
    int result=PoolAllocator_init(& _resources_allocator,
				  RESOURCE_SIZE,
				  MAX_NUM_RESOURCES,
				  _resources_buffer,
				  RESOURCE_BUFFER_SIZE);
    assert(! result);
}

Resource* Resource_alloc(int id, int type){
  Resource* r=(Resource*) PoolAllocator_getBlock(&_resources_allocator);
  if (!r)
    return 0;
  r->list.prev=r->list.next=0;
  r->id=id;
  r->type=type;
  List_init(&r->descriptors_ptrs);
  return r;
}

int Resource_free(Resource* r) {
  // TODO call queue_free in case resource is a queue
  assert(r->descriptors_ptrs.first==0);
  assert(r->descriptors_ptrs.last==0);
  return PoolAllocator_releaseBlock(&_resources_allocator, r);
}

Resource* ResourceList_byId(ResourceList* l, int id) {
  ListItem* aux=l->first;
  while(aux){
    Resource* r=(Resource*)aux;
    if (r->id==id)
      return r;
    aux=aux->next;
  }
  return 0;
}

// Queue resource section
void Queue_init(){
    int result=PoolAllocator_init(& _queues_allocator,
				  QUEUE_SIZE,
				  MAX_NUM_RESOURCES,
				  _queues_buffer,
				  QUEUE_BUFFER_SIZE);
    assert(! result);
}

Queue* Queue_alloc(){
  Queue* q=(Queue*) PoolAllocator_getBlock(&_queues_allocator);
  if (!q)
    return 0;
  List_init(&q->messages);
  List_init(&q->readers);
  List_init(&q->writers);
  List_init(&q->non_block);
  q->current_messages = 0;
  q->max_messages = DEFAULT_MAX_MESSAGES;
  q->msg_size = DEFAULT_MESSAGE_SIZE;
  // how many processes have opened the queue
  q->openings = 0;
  return q;
}

void Queue_add_pid(Queue* q, int pid, int mode){
    // TODO check if pid hasn't already opened the queue
    // TODO check that mode is at least one of rdonly, wronly
    if(mode & DSOS_RDONLY) List_insert(&q->readers,q->readers.last,(ListItem*)pid);
    if(mode & DSOS_WRONLY) List_insert(&q->writers,q->writers.last,(ListItem*)pid);
    if(mode & DSOS_NONBLOCK) List_insert(&q->non_block, q->non_block.last,(ListItem*)pid);
    q->openings ++;
}

int Queue_free(Queue* q) {
  // TODO check values
  return PoolAllocator_releaseBlock(&_queues_allocator, q);
}


// Debug

void Resource_print(Resource* r) {
  printf("id: %d, type:%d, pids:", r->id, r->type);
  DescriptorPtrList_print(&r->descriptors_ptrs);
}

void ResourceList_print(ListHead* l){
  ListItem* aux=l->first;
  printf("{\n");
  while(aux){
    Resource* r=(Resource*)aux;
    printf("\t");
    Resource_print(r);
    if(aux->next)
      printf(",");
    printf("\n");
    aux=aux->next;
  }
  printf("}\n");
}
