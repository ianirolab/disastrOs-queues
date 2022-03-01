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

#define MSG_STRING_SIZE sizeof(char) * MAX_MESSAGE_SIZE
#define MSG_STRING_MEMSIZE (MSG_STRING_SIZE +sizeof(int))
#define MSG_STRING_BUFFER_SIZE MAX_NUM_STRINGS*MSG_STRING_MEMSIZE

#define QUEUE_USER_SIZE sizeof(QueueUser)
#define QUEUE_USER_MEMSIZE (sizeof(QueueUser)+sizeof(int))
#define QUEUE_USER_BUFFER_SIZE MAX_NUM_PROCESSES*QUEUE_USER_MEMSIZE

#define MESSAGE_SIZE sizeof(Message)
#define MESSAGE_MEMSIZE (sizeof(Message)+sizeof(int))
#define MAX_NUM_MESSAGES (MAX_NUM_MESSAGES_PER_QUEUE*MAX_NUM_RESOURCES)
#define MESSAGE_BUFFER_SIZE MAX_NUM_MESSAGES*MESSAGE_MEMSIZE

static char _resources_buffer[RESOURCE_BUFFER_SIZE];
static PoolAllocator _resources_allocator;

static char _queues_buffer[QUEUE_BUFFER_SIZE];
static PoolAllocator _queues_allocator;

static char _msg_strings_buffer[MSG_STRING_BUFFER_SIZE];
static PoolAllocator _msg_strings_allocator;

static char _queue_users_buffer[QUEUE_BUFFER_SIZE];
static PoolAllocator _queue_users_allocator;

static char _message_buffer[MESSAGE_BUFFER_SIZE];
static PoolAllocator _message_allocator;


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
				  MAX_NUM_QUEUES,
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
  q->msg_size = MAX_MESSAGE_SIZE;
  // how many processes have opened the queue
  q->openings = 0;
  return q;
}

int Queue_free(Queue* q) {
  for (int i = 0; i < q->messages.size; i++){
    Message_free(List_pop(q->messages.first));
  }
  while(q->readers.first != 0) QueueUser_free(List_pop(q->readers.first));
  while(q->writers.first != 0) QueueUser_free(List_pop(q->writers.first));
  while(q->non_block.first != 0) QueueUser_free(List_pop(q->non_block.first));
  
  return PoolAllocator_releaseBlock(&_queues_allocator, q);
}


int Queue_add_pid(Queue* q, int pid, int mode, ListItem** ds){
    // It is not necessary to check if pid hasn't already opened the queue,
    // since dmq_open uses disastrOS_openResource, which doesn't allow a process
    // to open the same resource with multiple file descriptors
    
    if(mode & DSOS_RDONLY){ 
      QueueUser* qu = QueueUser_alloc(pid);
      ds[0] = List_insert(&q->readers,q->readers.last,(ListItem*)qu);
      if(ds[0] == 0) return LIST_INSERTION_ERROR;
    }
    if(mode & DSOS_WRONLY) {
      QueueUser* qu = QueueUser_alloc(pid);
      ds[1] = List_insert(&q->writers,q->writers.last,(ListItem*)qu);
      if(ds[1] == 0) return LIST_INSERTION_ERROR;
    }
    if(mode & DSOS_NONBLOCK){
      QueueUser* qu = QueueUser_alloc(pid);
      ds[2] = List_insert(&q->non_block, q->non_block.last,(ListItem*)qu);
      if(ds[2] == 0) return LIST_INSERTION_ERROR;
    } 
    q->openings ++;
    return 0;
}



void MsgString_init(){
    int result=PoolAllocator_init(& _msg_strings_allocator,
				  MSG_STRING_SIZE,
				  MAX_NUM_STRINGS,
				  _msg_strings_buffer,
				  MSG_STRING_BUFFER_SIZE);
    assert(! result);
}

MessageString MsgString_alloc(){
  MessageString ms=(MessageString) PoolAllocator_getBlock(&_msg_strings_allocator);
  if (!ms)
    return 0;
  return ms;
}

int MsgString_free(MessageString* m){
  return PoolAllocator_releaseBlock(&_msg_strings_allocator, m);
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

int QueueUser_free(QueueUser* qu){
  return PoolAllocator_releaseBlock(&_queue_users_allocator, qu);
}


void Message_init(){
  int result=PoolAllocator_init(& _message_allocator,
				MESSAGE_SIZE,
				MAX_NUM_PROCESSES,
				_message_buffer,
				MESSAGE_BUFFER_SIZE);
  assert(! result);
}

Message* Message_alloc(char* message, int message_size){
  Message* m=(Message*)PoolAllocator_getBlock(&_message_allocator);
  if(!m)
    return 0;
  m->message = message;
  m->len = message_size;
  return m;
}

int Message_free(Message* m){
  return PoolAllocator_releaseBlock(&_message_allocator, m) || MsgString_free(m->message);
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
