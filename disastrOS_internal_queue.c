#include <assert.h>
#include <stdio.h>
#include "disastrOS_resource.h"
#include "disastrOS_descriptor.h"
#include "pool_allocator.h"
#include "linked_list.h"
#include "disastrOS_internal_queue.h"

#define QUEUE_SIZE sizeof(Queue)
#define QUEUE_MEMSIZE (sizeof(Queue)+sizeof(int))
#define QUEUE_BUFFER_SIZE MAX_NUM_QUEUES*QUEUE_MEMSIZE

static char _queues_buffer[QUEUE_BUFFER_SIZE];
static PoolAllocator _queues_allocator;

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
  return q;
}

void Queue_add_pid(Queue* q, int pid, int mode){
    if(mode & DSOS_RDONLY) List_insert(&q->readers,q->readers.last,(ListItem*)pid);
    if(mode & DSOS_WRONLY) List_insert(&q->writers,q->writers.last,(ListItem*)pid);
    if(mode & DSOS_NONBLOCK) List_insert(&q->non_block, q->non_block.last,(ListItem*)pid);
}

int Queue_free(Queue* q) {
//   assert(q->descriptors_ptrs.first==0);
//   assert(q->descriptors_ptrs.last==0);
  return PoolAllocator_releaseBlock(&_queues_allocator, q);
}

// Resource* ResourceList_byId(ResourceList* l, int id) {
//   ListItem* aux=l->first;
//   while(aux){
//     Resource* r=(Resource*)aux;
//     if (r->id==id)
//       return r;
//     aux=aux->next;
//   }
//   return 0;
// }

// void Resource_print(Resource* r) {
//   printf("id: %d, type:%d, pids:", r->id, r->type);
//   DescriptorPtrList_print(&r->descriptors_ptrs);
// }

// void ResourceList_print(ListHead* l){
//   ListItem* aux=l->first;
//   printf("{\n");
//   while(aux){
//     Resource* r=(Resource*)aux;
//     printf("\t");
//     Resource_print(r);
//     if(aux->next)
//       printf(",");
//     printf("\n");
//     aux=aux->next;
//   }
//   printf("}\n");
// }
