#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <assert.h>

#include "disastrOS.h"
#include "disastrOS_queue.h"

#define CURRENT_TEST 9

// TODO make test look cleaner, printing only essential info, in a nice format

// we need this to handle the sleep state
void sleeperFunction(void* args){
  printf("Hello, I am the sleeper, and I sleep %d\n",disastrOS_getpid());
  while(1) {
    getc(stdin);
    disastrOS_printStatus();
  }
}

// child test functions compatible with test_queue_init
// Test 0: open a queue on two threads
void test_queue_child_0_0(void* args){
  int fd = dmq_open(0,DSOS_RDWR | DSOS_CREAT);
  printf("Queue opened with fd = %d\n",fd);
  disastrOS_printStatus();
  disastrOS_sleep(disastrOS_getpid() * 2);
  dmq_close(0);
  disastrOS_exit(disastrOS_getpid() + 1);
}

// Test 1: open one queue with 2 processes (WR/RD) send a message and print it. Process synchronization is handled outside disastrOS
void test_queue_child_1_0(void* args){
  int fd = dmq_open(0,DSOS_WRONLY | DSOS_CREAT);
  printf("Queue opened with fd = %d\n",fd);
  char* msg = "Hello world";
  int msg_len = 12;
  dmq_send(fd,msg,msg_len);
  // message should be in the queue
  disastrOS_printStatus();
  dmq_close(0);
  // this process disappears from the writers
  disastrOS_printStatus();
  disastrOS_exit(disastrOS_getpid() + 1);
}

void test_queue_child_1_1(void* args){
  int fd = dmq_open(0,DSOS_RDONLY);
  printf("Queue opened with fd = %d\n",fd);
  char buffer[128];
  int buffer_size = 128;
  // child added to the readers
  disastrOS_printStatus();
  dmq_receive(fd,buffer,buffer_size);
  dmq_close(0);
  printf("Message received: %s\n", buffer);
  // this process disappears from the readers
  disastrOS_printStatus();
  disastrOS_exit(disastrOS_getpid() + 1);
}

// Test 2: open one queue (WR/RD) send 2 messages and print them with 2 different processes. Process synchronization is handled outside disastrOS
void test_queue_child_2_0(void* args){
  int fd = dmq_open(0,DSOS_WRONLY | DSOS_CREAT);
  printf("Queue opened with fd = %d\n",fd);
  char* msg1 = "Hello world 1";
  char* msg2 = "Hello world 2";
  int msg_len = 14;
  dmq_send(fd,msg1,msg_len);
  dmq_send(fd,msg2,msg_len);
  // message should be in the queue
  disastrOS_printStatus();
  dmq_close(0);
  // this process disappears from the writers
  disastrOS_printStatus();
  disastrOS_exit(disastrOS_getpid() + 1);
}

// Test 3: open one queue (RD/WR) send a message and print it. The reader will be created before the writer and writer will wait some time
// to send the message
void test_queue_child_3_0(void* args){
  int fd = dmq_open(0,DSOS_WRONLY);
  printf("Queue opened by 3.0 with fd = %d\n",fd);
  char* msg = "Hello world";
  int msg_len = 12;
  disastrOS_sleep(10);
  dmq_send(fd,msg,msg_len);
  // message should be in the queue
  disastrOS_printStatus();
  dmq_close(0);
  disastrOS_exit(disastrOS_getpid() + 1);
}

void test_queue_child_3_1(void* args){
  int fd = dmq_open(0,DSOS_RDONLY | DSOS_CREAT);
  printf("Queue opened by 3.1 with fd = %d\n",fd);
  char buffer[128];
  int buffer_size = 128;
  // child added to the readers
  disastrOS_printStatus();
  // here process should wait until a message is available
  dmq_receive(fd,buffer,buffer_size);
  dmq_close(0);
  printf("Message received: %s\n", buffer);
  // this process disappears from the readers
  disastrOS_printStatus();
  disastrOS_exit(disastrOS_getpid() + 1);
}

// TODO: Openings doesn't return an int (what?)
// Test 4: test attributes getters and setters (without generating any errors)
void test_queue_child_4_0(void* args){
  int fd = dmq_open(0,DSOS_RDWR | DSOS_CREAT);
  printf("Queue opened with fd = %d\n",fd);
  printf("Printing attributes:\n");
  printf("Max messages: %d\n",dmq_getattr(fd,ATT_QUEUE_MAX_MESSAGES));
  printf("Message size: %d\n",dmq_getattr(fd,ATT_QUEUE_MESSAGE_SIZE));
  printf("Openings: %d\n"), dmq_getattr(fd, ATT_QUEUE_OPENINGS);
  printf("Message count: %d\n", dmq_getattr(fd,ATT_QUEUE_CURRENT_MESSAGES));

  char* msg = "Hello world";
  int msg_len = 12;
  dmq_send(fd,msg,msg_len);

  printf("\nEditing attributes...\n");
  int new_max_messages = 10;
  int new_message_size = 8;
  dmq_setattr(fd,ATT_QUEUE_MAX_MESSAGES,new_max_messages);
  dmq_setattr(fd,ATT_QUEUE_MESSAGE_SIZE,new_message_size);
  printf("\nPrinting edited attributes\n");
  printf("Max messages: %d\n",dmq_getattr(fd,ATT_QUEUE_MAX_MESSAGES));
  printf("Message size: %d\n",dmq_getattr(fd,ATT_QUEUE_MESSAGE_SIZE));
  printf("Openings: %d\n"), dmq_getattr(fd, ATT_QUEUE_OPENINGS);
  printf("Message count after sending a message: %d\n", dmq_getattr(fd, ATT_QUEUE_CURRENT_MESSAGES));
  dmq_close(0);
  disastrOS_exit(disastrOS_getpid() + 1);
}

// Test 5: open one queue (WR/RD) fill the queue with 'n' messages and send another message over the queue limit (n+1). 
// Then read the message from the full queue and let the writer write the (n+1)th messsage
void test_queue_child_5_0(void* args){
  int fd = dmq_open(0,DSOS_WRONLY | DSOS_CREAT);
  printf("Queue opened by 5.0 with fd = %d\n",fd);
  char* msg = "Hello world";
  int msg_len = 12;
  int new_max_messages = 10;
  dmq_setattr(fd,ATT_QUEUE_MAX_MESSAGES,new_max_messages);
  for(int i = 0; i < new_max_messages; i++){
    dmq_send(fd,msg,msg_len);
  }
  disastrOS_printStatus();
  // send the extra message
  char* extra_msg = "Extra Hello world";
  int extra_msg_len = 18;
  dmq_send(fd,extra_msg,extra_msg_len);
  disastrOS_printStatus();
  dmq_close(0);
  disastrOS_exit(disastrOS_getpid() + 1);
}

void test_queue_child_5_1(void* args){
  int fd = dmq_open(0,DSOS_RDONLY);
  printf("Queue opened by 5.1 with fd = %d\n",fd);

  char buffer[128];
  int buffer_size = 128;
  disastrOS_sleep(10);
  disastrOS_printStatus();
  dmq_receive(fd,buffer,buffer_size);
  dmq_close(0);
  printf("Message received: %s\n", buffer);
  // this process disappears from the readers
  disastrOS_printStatus();
  disastrOS_exit(disastrOS_getpid() + 1);
}

// Test 6: open a queue in DSOS_EXCL (without generating any errors)
void test_queue_child_6_0(void* args){
  int fd = dmq_open(0,DSOS_RDWR | DSOS_CREAT | DSOS_Q_EXCL);
  printf("Queue opened by 6.0 with fd = %d\n",fd);
  disastrOS_printStatus();
  disastrOS_exit(disastrOS_getpid() + 1);
}

// Test 7: open a queue (RDWR,NONBLOCK) and try to read from the empty queue. Then fill it and try to send a message when queue is full
void test_queue_child_7_0(void* args){
  int fd = dmq_open(0,DSOS_RDWR | DSOS_CREAT | DSOS_NONBLOCK);
  printf("Queue opened by 7.0 with fd = %d\n",fd);
  // try to read empty queue
  int retval;
  int buffer_size = 128;
  char buffer[buffer_size];

  retval = dmq_receive(fd,buffer,buffer_size);
  printf("receive assertion passed!\n");
  assert(retval == EAGAIN);

  char* msg = "Hello world";
  int msg_len = 12;
  int new_max_messages = 10;
  dmq_setattr(fd,ATT_QUEUE_MAX_MESSAGES,new_max_messages);
  for(int i = 0; i < new_max_messages; i++){
    dmq_send(fd,msg,msg_len);
  }
  // send the extra message
  char* extra_msg = "Extra Hello world";
  int extra_msg_len = 18;
  retval = dmq_send(fd,extra_msg,extra_msg_len);
  disastrOS_printStatus();
  assert(retval == EAGAIN);
  printf("send assertion passed!\n");
  dmq_close(0);
  disastrOS_exit(disastrOS_getpid() + 1);
}

// Test8: open a queue, and exit child without closing the queue (exit should handle the queue closing, and shutdown the unlink)
void test_queue_child_8_0(void* args){
  int fd = dmq_open(0,DSOS_RDWR | DSOS_CREAT);
  printf("Queue opened with fd = %d\n",fd);
  disastrOS_printStatus();
  disastrOS_sleep(disastrOS_getpid() * 2);
  disastrOS_exit(disastrOS_getpid() + 1);
}

// Test9: test maximum values for queues and messages
void test_queue_child_9_0(void* args){
  int lastRet = 0;
  for (int i = 0; i <= MAX_NUM_RESOURCES; i++){
      lastRet = dmq_open(i,DSOS_CREAT);
  }
  printf("Last queue opened with fd: %d\n",lastRet);
  printf("Update of first queue's attribute queue_max_messages over the limit returned: %d\n",
  dmq_setattr(0,ATT_QUEUE_MAX_MESSAGES,MAX_NUM_MESSAGES_PER_QUEUE * 2));
  disastrOS_exit(disastrOS_getpid() + 1);
}


// TODO Test x: and errors (creation(EXCL) errors, ...), advanced test (multiple queues, users, random stuffs),  
// make test output look cleaner

void test_queue_init(void* args){
  disastrOS_printStatus();
  printf("Starting queue tests\n");
  disastrOS_spawn(sleeperFunction,0);

  // Test switch
  int alive_children;

  switch (CURRENT_TEST){
  case 0:
    printf("Test0: spawning 2 processes, that open the same queue and close it after some time\n");
    disastrOS_spawn(test_queue_child_0_0, 0);
    disastrOS_spawn(test_queue_child_0_0, 0);
    alive_children = 2;
    break;
  
  case 1:
    printf("Test1: spawning 1 process, that opens a queue, leaves a message and closes it\n");
    disastrOS_spawn(test_queue_child_1_0, 0);

    alive_children = 1;
    break;
  
  case 2:
    printf("Test2: spawning 1 process, that opens a queue, leaves 2 messages and closes it\n");
    disastrOS_spawn(test_queue_child_2_0, 0);

    alive_children = 1;
    break;

  case 3:
    printf("Test3: spawning 1 process, that opens an empty queue, and thus has to wait \n");
    disastrOS_spawn(test_queue_child_3_1, 0);
    disastrOS_spawn(test_queue_child_3_0, 0);

    alive_children = 2;
    break;
  
  case 4:
    printf("Test4: spawning 1 process, that opens an empty queue, and prints its attributes before and after sending a message \n");
    disastrOS_spawn(test_queue_child_4_0, 0);

    alive_children = 1;
    break;

  case 5:
    printf("Test5: spawning 2 processes, a reader and a writer. Writer will fill the queue and try to send an extra message\n");
    printf("but will have to wait until the reader reads a message");
    disastrOS_spawn(test_queue_child_5_0, 0);
    disastrOS_spawn(test_queue_child_5_1, 0);

    alive_children = 2;
    break;

  case 6: 
    printf("Test6: spawning a process that opens the queue with flags O_CREAT and O_Q_EXCL\n");
    disastrOS_spawn(test_queue_child_6_0,0);

    alive_children = 1;
    break;
  
  case 7:
    printf("Test7: spawning a process that opens a non-blocking queue, tries to read from the empty queue and tries to write to the full queue \n");
    disastrOS_spawn(test_queue_child_7_0,0);

    alive_children = 1;
    break;
  
  case 8:
    printf("Test8: spawning a process that opens a queue, and exits without closing it \n");
    disastrOS_spawn(test_queue_child_8_0,0);

    alive_children = 1;
    break;
  
  case 9:
    printf("Test9: testing behaviour when trying to create a number of queues > MAX_NUM_RESOURCES, and setting the value of queue->max_messages \n");
    printf("\tto a number greater than MAX_NUM_MESSAGES_PER_QUEUE\n");
    disastrOS_spawn(test_queue_child_9_0,0);
    
    alive_children = 1;
    break;

  default:
    break;
  }

  int pid;
  int retval;
  while(alive_children>0 && (pid=disastrOS_wait(0, &retval))>=0){ 
    // disastrOS_printStatus();
    printf("test_queue_init, child: %d terminated, retval:%d, alive: %d \n",pid, retval, alive_children);
    --alive_children;
  }


  // aftertests
  switch (CURRENT_TEST){
  case 0:
    dmq_unlink(0,0);
    break;
  case 1:
    printf("Test 1: spawning another child that opens the previous queue, reads and prints the message and exits\n");
    disastrOS_spawn(test_queue_child_1_1,0);
    disastrOS_wait(0,&retval);
    dmq_unlink(0,0);
    break;
  case 2:
    printf("Test 2: spawning 2 children that open the previous queue, read and print one message each and exit\n");
    printf("Child 1: \n");
    disastrOS_spawn(test_queue_child_1_1,0);
    disastrOS_wait(0,&retval);
    printf("Child 2: \n");
    disastrOS_spawn(test_queue_child_1_1,0);
    disastrOS_wait(0,&retval);
    dmq_unlink(0,0);
    break;
  case 3:
    disastrOS_printStatus();
    dmq_unlink(0,0);
    break;
  case 4:
    dmq_unlink(0,0);
    break;
  case 5:
    dmq_unlink(0,0);
    break;
  case 6:
    dmq_unlink(0,0);
    break;
  case 7:
    dmq_unlink(0,0);
    break;
  case 8:
    disastrOS_printStatus();
    break;

  default:

    break;
  }

  printf("Shutdown\n");
  disastrOS_shutdown();

}

int main(int argc, char** argv){
  char* logfilename=0;
  if (argc>1) {
    logfilename=argv[1];
  }
  // we create the init process processes
  // the first is in the running variable
  // the others are in the ready queue
  // printf("the function pointer is: %p", childFunction);
  // spawn an init process
  printf("start\n");
  disastrOS_start(test_queue_init, 0, logfilename);
  return 0;
}
