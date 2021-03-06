#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <assert.h>
#include <time.h> //used only to generate random numbers
#include <stdlib.h>

#include "disastrOS.h"
#include "disastrOS_queue.h"

#define TEST_10_QUEUES_PER_CHILD 10
// determines how much time the processes have until they all get killed.
// Since test 10 generates a lot of things randomly there is the chance that
// either a reader waits forever on an empty queue or a writer waits forever
// on a full queue. Since the goal of the test it to generate a lot of random
// queue movements to check wheter the implementation is consistent (the result of the
// operations has to be correct) and stable (running this test multiple 
// times might create a segfault or an error of the kind). The test doesn't have to
// ensure that all writers get an non-full queue and all readers get a non-empty queue.
// So it's useful to have a countdown to kill processes that are stuck.

#define TEST_10_KILLER_SLEEP 200
// default test
int CURRENT_TEST = 0;

const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";  //used in test10


// we need this to handle the sleep state
void sleeperFunction(void* args){
  // printf("Hello, I am the sleeper, and I sleep %d\n",disastrOS_getpid());
  while(1) {
    getc(stdin);
    disastrOS_printStatus();
  }
}

// child test functions compatible with test_queue_init

// Test 0: open a queue on two threads
void test_queue_child_0_0(void* args){
  int fd = dmq_open(0,DSOS_RDWR | DSOS_CREAT);
  printf("Queue opened with fd = %d, in RDWR mode\n",fd);
  disastrOS_printStatus();
  disastrOS_sleep(disastrOS_getpid() * 2);
  dmq_close(fd);
  disastrOS_exit(0);
}

// Test 1: open one queue with 2 processes (WR/RD) send a message and print it. Process synchronization is handled outside disastrOS
void test_queue_child_1_0(void* args){
  int fd = dmq_open(0,DSOS_WRONLY | DSOS_CREAT);
  printf("Queue opened with fd = %d\n",fd);
  char* msg = "Hello world";
  int msg_len = 12;
  dmq_send(fd,msg,msg_len);
  // message should be in the queue
  printf("Message sent to queue\n");
  disastrOS_printStatus();
  dmq_close(fd);
  disastrOS_exit(0);
}

void test_queue_child_1_1(void* args){
  int fd = dmq_open(0,DSOS_RDONLY);
  printf("Queue opened with fd = %d\n",fd);
  char buffer[128];
  int buffer_size = 128;
  dmq_receive(fd,buffer,buffer_size);
  dmq_close(fd);
  printf("Message from queue has been read, queue closed\n");
  printf("Message received: %s\n", buffer);
  disastrOS_printStatus();
  disastrOS_exit(0);
}

// Test 2: open one queue (WR/RD) send 2 messages and print them with 2 different processes. Process synchronization is handled outside disastrOS
void test_queue_child_2_0(void* args){
  int fd = dmq_open(0,DSOS_WRONLY);
  printf("Queue opened with fd = %d\n",fd);
  printf("Sending 2 messages to the queue...\n");
  char* msg1 = "Hello world 1";
  char* msg2 = "Hello world 2";
  int msg_len = 14;
  dmq_send(fd,msg1,msg_len);
  dmq_send(fd,msg2,msg_len);
  dmq_close(fd);
  printf("Messages sent and queue closed. Showing queue status\n");
  disastrOS_printStatus();
  disastrOS_exit(0);
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
  dmq_close(fd);
  // message should be in the queue
  disastrOS_printStatus();
  disastrOS_exit(0);
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
  dmq_close(fd);
  printf("Message received: %s\n", buffer);
  // this process disappears from the readers
  disastrOS_printStatus();
  disastrOS_exit(0);
}

// Test 4: test attributes getters and setters (without generating any errors)
void test_queue_child_4_0(void* args){
  int fd = dmq_open(0,DSOS_RDWR | DSOS_CREAT);
  printf("Queue opened with fd = %d\n",fd);
  printf("Printing attributes:\n");
  printf("Max messages: %d\n",dmq_getattr(fd,ATT_QUEUE_MAX_MESSAGES));
  printf("Message size: %d\n",dmq_getattr(fd,ATT_QUEUE_MESSAGE_SIZE));
  printf("Openings: %d\n", dmq_getattr(fd, ATT_QUEUE_OPENINGS));
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
  printf("Openings: %d\n", dmq_getattr(fd, ATT_QUEUE_OPENINGS));
  printf("Message count after sending a message: %d\n", dmq_getattr(fd, ATT_QUEUE_CURRENT_MESSAGES));
  dmq_close(fd);
  disastrOS_exit(0);
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
  dmq_close(fd);
  disastrOS_exit(0);
}

void test_queue_child_5_1(void* args){
  int fd = dmq_open(0,DSOS_RDONLY);
  printf("Queue opened by 5.1 with fd = %d\n",fd);

  char buffer[128];
  int buffer_size = 128;
  printf("Waiting time = 10 before reading\n");
  disastrOS_sleep(10);
  dmq_receive(fd,buffer,buffer_size);
  dmq_close(fd);
  printf("Message received: %s\n", buffer);
  // this process disappears from the readers
  disastrOS_printStatus();
  disastrOS_exit(0);
}

// Test 6: open a queue in DSOS_EXCL (without generating any errors)
void test_queue_child_6_0(void* args){
  int fd = dmq_open(0,DSOS_RDWR | DSOS_CREAT | DSOS_Q_EXCL);
  printf("Queue opened by 6.0 with fd = %d\n",fd);
  disastrOS_printStatus();
  dmq_close(fd);
  disastrOS_exit(0);
}

void test_queue_child_6_1(void* args){
  // wait to make sure that the queue has been opened
  disastrOS_sleep(5);
  int fd = dmq_open(0,DSOS_RDWR | DSOS_CREAT | DSOS_Q_EXCL);
  printf("Queue opened by 6.1 with fd = %d\n",fd);
  printf("dmq_close returned: %d\n",dmq_close(fd));
  disastrOS_exit(0);
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
  dmq_close(fd);
  disastrOS_exit(0);
}

// Test8: open a queue, and exit child without closing the queue (exit should handle the queue closing, and shutdown the unlink)
void test_queue_child_8_0(void* args){
  int fd = dmq_open(0,DSOS_RDWR | DSOS_CREAT);
  printf("Queue opened with fd = %d\n",fd);
  disastrOS_printStatus();
  disastrOS_sleep(disastrOS_getpid() * 2);
  disastrOS_exit(0);
}

// Test9: test maximum values for queues and messages
void test_queue_child_9_0(void* args){
  int lastRet = 0;
  for (int i = 0; i < MAX_NUM_QUEUES; i++){
    lastRet = dmq_open(i,DSOS_CREAT);
  }
  lastRet = dmq_open(MAX_NUM_QUEUES, DSOS_CREAT);
  printf("Last queue opened with fd: %d\n",lastRet);
  printf("Update of first queue's attribute queue_max_messages over the limit returned: %d\n",
  dmq_setattr(0,ATT_QUEUE_MAX_MESSAGES,MAX_NUM_MESSAGES_PER_QUEUE * 2));
  // the queues are all closed by disastrOS_shutdown as in test8
  disastrOS_exit(0);
}

void test_queue_child_10_0_read(int fd, int pid, int queue_id){
  char buffer[MAX_MESSAGE_SIZE];
  int res = dmq_receive(fd, buffer,MAX_MESSAGE_SIZE);
  if (res) printf("Process %d: error on dmq_receive [%d]\n", pid, res);
  else printf("Process %d: has received the message: '%s' on queue: %d\n", pid, buffer, queue_id);
}

void test_queue_child_10_0_write(int fd, int pid, int queue_id){
  disastrOS_printStatus();
  int str_len = rand() % (MAX_MESSAGE_SIZE - 1) + 1;
  char str_built [str_len]; 
  for (int k = 0; k < str_len; k++) {
      int key = rand() % (int) (sizeof charset - 1);
      str_built[k] = charset[key];
  }
  str_built[str_len] = '\0';
  int res = dmq_send(fd, str_built, str_len);

  
  if (res) printf("Process %d: error on dmq_send [%d]\n", pid, res);
  else printf("Process %d: has sent the message: '%s' on queue: %d\n", pid, str_built, queue_id);
}

void test_queue_child_10_0_read_write(int fd, int pid, int queue_id){
  disastrOS_printStatus();
  int read_flag = rand() % 101 > 50;
  if (read_flag) test_queue_child_10_0_read(fd, pid, queue_id);
  else test_queue_child_10_0_write(fd, pid, queue_id);
}

void test_10_killer(){
  disastrOS_sleep(TEST_10_KILLER_SLEEP);
  printf("Killing stuck processes\n");
  disastrOS_shutdown();
}

// Test10: testing behaviour of disastrOS when opening multiple queues, on multiple processes, with random sleep() and random process role 
// (RW/WR/RD), eventually handling errors
void test_queue_child_10_0(void* args){
  printf("Starting process %d\n", disastrOS_getpid());
  const int sleep_max = 10;
  const int operation_max = 3;

  int* queue_ids = (int*) args;
  
  printf("queues: [");
  for (int i = 0; i < TEST_10_QUEUES_PER_CHILD; i++){
    printf("%d, ", queue_ids[i]);
  }
  printf("]\n");

  for (int i = 0; i < TEST_10_QUEUES_PER_CHILD; i++){
    int opening_mode = (rand() % 3); //0: read, 1: write, 2: read/write
    int exclusive = (rand() % 101) > 5; // 5% chance of opening the queue in DSOS_EXCL (will likely generate an error)
    int open_flags = exclusive ? DSOS_CREAT | DSOS_EXCL : DSOS_CREAT;
    
    switch (opening_mode){
    case 0:
      open_flags |= DSOS_RDONLY;
      break;
    
    case 1:
      open_flags |= DSOS_WRONLY;
      break;

    case 2:
      open_flags |= DSOS_RDWR;
      break;

    default:
      break;
    }    
  
    int fd = dmq_open(queue_ids[i],open_flags);
    int pid = disastrOS_getpid();
    if (fd < 0) {
      printf("Process %d: error on dmq_open [%d], trying to open queue %d in mode %d. Exclusive = %d\n", pid, fd, queue_ids[i],opening_mode,exclusive);
      continue;
    }
  
    for(int j = 0; j < operation_max; j++){
      printf("Starting operation %d for process %d\n", j, pid);
      
      switch (opening_mode){
      case 0:
        printf("Process %d is a reader process\n",pid);
        test_queue_child_10_0_read(fd,pid,queue_ids[i]);
        break;
      
      case 1:
        printf("Process %d is a writer process, opening %d\n",pid, queue_ids[i]);
        test_queue_child_10_0_write(fd,pid,queue_ids[i]);
        break;

      case 2:
        printf("Process %d is a reader/writer process\n",pid);
        test_queue_child_10_0_read_write(fd, pid,queue_ids[i]);
        break;

      default:
        break;
      }    
      disastrOS_sleep(rand()%sleep_max);
    }
    dmq_close(fd);
  }
  disastrOS_exit(0);
}


void test_queue_init(void* args){
  printf("Starting queue tests\n");
  disastrOS_spawn(sleeperFunction,0);

  // Test switch
  int alive_children;
  int fd;
  switch (CURRENT_TEST){
  case 0:
    printf("Test0: spawning 2 processes, that open the same queue and close it after some time\n");
    fd = dmq_open(0,DSOS_CREAT);
    disastrOS_spawn(test_queue_child_0_0, 0);
    disastrOS_spawn(test_queue_child_0_0, 0);
    alive_children = 2;
    break;
  
  case 1:
    printf("Test1: spawning 1 process, that opens a queue, leaves a message and closes it\n");
    fd = dmq_open(0,DSOS_CREAT);
    disastrOS_spawn(test_queue_child_1_0, 0);

    alive_children = 1;
    break;
  
  case 2:
    printf("Test2: spawning 1 process, that opens a queue, leaves 2 messages and closes it\n");
    fd = dmq_open(0,DSOS_CREAT);
    disastrOS_spawn(test_queue_child_2_0, 0);

    alive_children = 1;
    break;

  case 3:
    printf("Test3: spawning 1 process, that opens an empty queue, and thus has to wait \n");
    fd = dmq_open(0,DSOS_CREAT);
    disastrOS_spawn(test_queue_child_3_1, 0);
    disastrOS_spawn(test_queue_child_3_0, 0);

    alive_children = 2;
    break;
  
  case 4:
    printf("Test4: spawning 1 process, that opens an empty queue, and prints its attributes before and after sending a message \n");
    fd = dmq_open(0,DSOS_CREAT);
    disastrOS_spawn(test_queue_child_4_0, 0);

    alive_children = 1;
    break;

  case 5:
    printf("Test5: spawning 2 processes, a reader and a writer. Writer will fill the queue and try to send an extra message\n");
    printf("but will have to wait until the reader reads a message\n");
    fd = dmq_open(0,DSOS_CREAT);
    disastrOS_spawn(test_queue_child_5_0, 0);
    disastrOS_spawn(test_queue_child_5_1, 0);

    alive_children = 2;
    break;

  case 6: 
    printf("Test6: spawning a process that opens the queue with flags O_CREAT and O_Q_EXCL\n");
    disastrOS_spawn(test_queue_child_6_0,0);
    printf("Test6: now spawining another process that tries to open the queue with the same flages as before\n");
    disastrOS_spawn(test_queue_child_6_1,0);

    alive_children = 2;
    break;
  
  case 7:
    printf("Test7: spawning a process that opens a non-blocking queue, tries to read from the empty queue and tries to write to the full queue \n");
    fd = dmq_open(0,DSOS_CREAT);
    disastrOS_spawn(test_queue_child_7_0,0);

    alive_children = 1;
    break;
  
  case 8:
    printf("Test8: spawning a process that opens a queue, and exits without closing it nor unlinking it \n");
    disastrOS_spawn(test_queue_child_8_0,0);

    alive_children = 1;
    break;
  
  case 9:
    printf("Test9: testing behaviour when trying to create a number of queues > MAX_NUM_QUEUES, and setting the value of queue->max_messages \n");
    printf("\tto a number greater than MAX_NUM_MESSAGES_PER_QUEUE\n");
    disastrOS_spawn(test_queue_child_9_0,0);
    
    alive_children = 1;
    break;

  case 10:
    printf("Test10: testing behaviour of disastrOS when opening multiple queues, on multiple processes, with random sleep() and random process role (RW/WR/RD), eventually handling errors\n");
    srand(time(NULL));
    // int num_proccesses = (rand() % 1076) + 32;
    int num_proccesses = 100;
    for (int i = 0; i < num_proccesses; i++){
      // array of queue_ids 
      // the way each queue will be opened and the duration of its sleep will be decided inside test_queue_child_10_0
      int queue_ids[TEST_10_QUEUES_PER_CHILD]; 
      int last_id = 0;
      
      for (int j = 0; j < TEST_10_QUEUES_PER_CHILD; j++){
        last_id = rand() % TEST_10_QUEUES_PER_CHILD + 1;
        queue_ids[j] = last_id;
      }

      disastrOS_spawn(test_queue_child_10_0, queue_ids);
    }
    alive_children = num_proccesses;
    disastrOS_spawn(test_10_killer,0);
    break;

  default:
    break;
  }

  int pid;

  int retval;
  while(alive_children>0 && (pid=disastrOS_wait(0, &retval))>=0){ 
    printf("test_queue_init, child: %d terminated, retval:%d, alive: %d \n",pid, retval, --alive_children);
  }


  // aftertests
  switch (CURRENT_TEST){
  case 0:
    dmq_close(fd);
    dmq_unlink(0);
    printf("All children from test %d have terminated and queue has been unlinked\n", CURRENT_TEST);
    break;
  case 1:
    printf("Test 1: spawning another child that opens the previous queue, reads and prints the message and exits\n");
    disastrOS_spawn(test_queue_child_1_1,0);
    disastrOS_wait(0,&retval);
    dmq_close(fd);
    dmq_unlink(0);
    printf("All children from test %d have terminated and queue has been unlinked\n", CURRENT_TEST);
    break;
  case 2:
    printf("Test 2: spawning 2 children that open the previous queue, read and print one message each and exit\n");
    printf("Child 1: \n");
    disastrOS_spawn(test_queue_child_1_1,0);
    disastrOS_wait(0,&retval);
    printf("Child 2: \n");
    disastrOS_spawn(test_queue_child_1_1,0);
    disastrOS_wait(0,&retval);
    dmq_close(fd);
    dmq_unlink(0);
    printf("All children from test %d have terminated and queue has been unlinked\n", CURRENT_TEST);
    break;
  case 3:
    disastrOS_printStatus();
    dmq_close(fd);
    dmq_unlink(0);
    printf("All children from test %d have terminated and queue has been unlinked\n", CURRENT_TEST);
    break;
  case 4:
    dmq_close(fd);
    dmq_unlink(0);
    printf("All children from test %d have terminated and queue has been unlinked\n", CURRENT_TEST);
    break;
  case 5:
    dmq_close(fd);
    dmq_unlink(0);
    printf("All children from test %d have terminated and queue has been unlinked\n", CURRENT_TEST);
    break;
  case 6:
    dmq_unlink(0);
    printf("All children from test %d have terminated and queue has been unlinked\n", CURRENT_TEST);
    break;
  case 7:
    dmq_close(fd);
    dmq_unlink(0);
    printf("All children from test %d have terminated and queue has been unlinked\n", CURRENT_TEST);
    break;
  case 8:
    disastrOS_printStatus();
    printf("All children from test %d have terminated and queue will be closed and unlinked on shutdown\n", CURRENT_TEST);
    break;
  case 9:
    dmq_unlink(0);
    printf("All children from test %d have terminated and queue has been unlinked\n", CURRENT_TEST);
    break;
  case 10:
    for (int i = 0; i < TEST_10_QUEUES_PER_CHILD; i++){
      dmq_unlink(i);
    }
    printf("All children from test %d have terminated\n", CURRENT_TEST);
    break;

  default:

    break;
  }

  printf("Shutdown\n");
  while(disastrOS_shutdown() == DSOS_SHUTTING) sleep(1);
  // disastrOS_shutdown();
  disastrOS_printStatus();
}

int main(int argc, char** argv){
  char* logfilename=0;
  if (argc>1) {
    // logfilename=argv[1];
    CURRENT_TEST = atoi(argv[1]);
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
