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
#define QUEUE_BUFFER_SIZE MAX_NUM_RESOURCES*QUEUE_MEMSIZE

#define QUEUE_USER_SIZE sizeof(QueueUser)
#define QUEUE_USER_MEMSIZE (sizeof(QueueUser)+sizeof(int))
#define QUEUE_USER_BUFFER_SIZE MAX_NUM_PROCESSES*QUEUE_USER_MEMSIZE

#define MESSAGE_SIZE sizeof(Message)
#define MESSAGE_MEMSIZE (sizeof(Message)+sizeof(int))
#define MAX_NUM_MESSAGES (MAX_NUM_MESSAGES_PER_QUEUE*MAX_NUM_RESOURCES)
#define MESSAGE_BUFFER_SIZE MAX_NUM_MESSAGES*MESSAGE_MEMSIZE

#define MESSAGEPTR_SIZE sizeof(MessagePtr)
#define MESSAGEPTR_MEMSIZE (sizeof(MessagePtr)+sizeof(int))
#define MESSAGEPTR_BUFFER_SIZE MAX_NUM_MESSAGES*MESSAGEPTR_MEMSIZE

static char _resources_buffer[RESOURCE_BUFFER_SIZE];
static PoolAllocator _resources_allocator;

static char _queues_buffer[QUEUE_BUFFER_SIZE];
static PoolAllocator _queues_allocator;

static char _queue_users_buffer[QUEUE_BUFFER_SIZE];
static PoolAllocator _queue_users_allocator;


static char _message_buffer[MESSAGE_BUFFER_SIZE];
static PoolAllocator _message_allocator;

static char _message_ptr_buffer[MESSAGEPTR_BUFFER_SIZE];
static PoolAllocator _message_ptr_allocator;

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

Queue* Queue_alloc(int resource_id){
  Queue* q=(Queue*) PoolAllocator_getBlock(&_queues_allocator);
  if (!q)
    return 0;
  List_init(&q->messages);
  List_init(&q->readers);
  List_init(&q->writers);
  List_init(&q->non_block);
  q->max_messages = MAX_NUM_MESSAGES_PER_QUEUE;
  q->msg_size = DEFAULT_MESSAGE_SIZE;
  // how many processes have opened the queue
  q->openings = 0;
  q->unlink_request = 0;
  q->resource_id = resource_id;
  return q;
}

void Queue_add_pid(Queue* q, int pid, int mode, ListItem** ds){
    // TODO check if pid hasn't already opened the queue
    
    if(mode & DSOS_RDONLY){ 
      QueueUser* qu = QueueUser_alloc(pid);
      ds[0] = List_insert(&q->readers,q->readers.last,(ListItem*)qu);
      // TODO check that value is not 0  
    }
    if(mode & DSOS_WRONLY) {
      QueueUser* qu = QueueUser_alloc(pid);
      ds[1] = List_insert(&q->writers,q->writers.last,(ListItem*)qu);
    }
    if(mode & DSOS_NONBLOCK){
      QueueUser* qu = QueueUser_alloc(pid);
      ds[2] = List_insert(&q->non_block, q->non_block.last,(ListItem*)qu);
    } 
    q->openings ++;
}

int Queue_free(Queue* q) {
  // TODO check values
  return PoolAllocator_releaseBlock(&_queues_allocator, q);
}

void QueueUser_init(){
  int result=PoolAllocator_init(& _queue_users_allocator,
				  QUEUE_USER_SIZE,
				  MAX_NUM_PROCESSES,
				  _queue_users_buffer,
				  QUEUE_USER_BUFFER_SIZE);
    assert(! result);
}

QueueUser* QueueUser_alloc(int pid){
  QueueUser* qu =(QueueUser*) PoolAllocator_getBlock(&_queue_users_allocator);
  if (!qu)
    return 0;
  qu->pid = pid;
  qu->status = Running;
  return qu;
}

void Message_init(){
  int result=PoolAllocator_init(& _message_allocator,
				MESSAGE_SIZE,
				MAX_NUM_PROCESSES,
				_message_buffer,
				MESSAGE_BUFFER_SIZE);
  assert(! result);

  result=PoolAllocator_init(& _message_ptr_allocator,
			    MESSAGEPTR_SIZE,
			    MAX_NUM_PROCESSES,
			    _message_ptr_buffer,
			    MESSAGEPTR_BUFFER_SIZE);
  assert(! result);
}

Message* Message_alloc(const char* message, int message_size){
  Message* m=(Message*)PoolAllocator_getBlock(&_message_allocator);
  if(!m)
    return 0;
  // TODO copy the string
  m->message = message;
  m->len = message_size;
  return m;
}

MessagePtr* MessagePtr_alloc(Message* message){
  MessagePtr* m=(MessagePtr*)PoolAllocator_getBlock(&_message_ptr_allocator);
  if (!m)
    return 0;
  // TODO maybe copy the string
  m->list.prev = m->list.next = 0;
  m->message = message;
  return m;
}


// Debug

void Resource_print(Resource* r) {
  printf("id: %d, type:%d, pids:", r->id, r->type);
  if (r->type == 2)
    Queue_print((Queue*)r->value);
  else
    DescriptorPtrList_print(&r->descriptors_ptrs);
}

void Queue_print(Queue* q){
  printf("\n");
  if (PRINT_QUEUE_MESSAGES){
    ListItem* aux = q->messages.first;
    for (int i = 0; i < q->messages.size; i++){
      printf("\tMessage %d: %s\n",i,((Message*)aux)->message);
      aux = aux->next;
    }
    
  }
  if (PRINT_QUEUE_READERS){
    ListItem* aux = q->readers.first;
    printf("\t\tReaders: [");
    for (int i = 0; i < q->readers.size; i++){
      printf("%d",((QueueUser*)aux)->pid);
      if (aux->next)
        printf(", ");
      aux = aux->next;
    }
    printf("]\n");
  }
  if (PRINT_QUEUE_WRITERS){
    ListItem* aux = q->writers.first;
    printf("\t\tWriters: [");
    for (int i = 0; i < q->writers.size; i++){
      printf("%d",((QueueUser*)aux)->pid);
      if (aux->next)
        printf(", ");
      aux = aux->next;
    }
    printf("]\n");
  }
  if (PRINT_QUEUE_NONBLOCK){
    ListItem* aux = q->non_block.first;
    printf("\t\tNon-Block: [");
    for (int i = 0; i < q->non_block.size; i++){
      printf("%d",((QueueUser*)aux)->pid);
      if (aux->next)
        printf(", ");
      aux = aux->next;
    }
    printf("]\n");
  }

  if (PRINT_QUEUE_ATTRIBUTES){
    printf("\t\tAttributes: Max messages: %d, Message size: %d,Openings: %d, Messages count: %d",
      q->max_messages,q->msg_size, q->openings, q->messages.size);
  }
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
