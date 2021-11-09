#include <stdio.h>
#include <unistd.h>
#include <poll.h>

#include "disastrOS.h"
#include "disastrOS_queue.h"

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
void test_queue_child_0_1(void* args){
  int fd = dmq_open(0,DSOS_RDWR);
  printf("Queue opened with fd = %d\n",fd);
  disastrOS_printStatus();
  disastrOS_sleep(disastrOS_getpid() * 2);
  dmq_close(0);
  disastrOS_exit(disastrOS_getpid() + 1);
}

// Test 1: open two queues (WR/RD) send a message and print it. Process synchronization is handled outside disastrOS
void test_queue_child_1_0(void* args){
  int fd = dmq_open(0,DSOS_WRONLY | DSOS_CREAT);
  printf("Queue opened with fd = %d\n",fd);
  char* msg = "Hello world";
  int msg_len = 12;
  dmq_send(fd,msg,msg_len);
  dmq_close(0);
  disastrOS_exit(disastrOS_getpid() + 1);
}

void test_queue_child_1_1(void* args){
  int fd = dmq_open(0,DSOS_RDONLY);
  printf("Queue opened with fd = %d\n",fd);
  char buffer[20];
  int buffer_size = 20;
  dmq_receive(fd,buffer,buffer_size);
  dmq_close(0);
  printf("Message received: %s\n", buffer);
  disastrOS_exit(disastrOS_getpid() + 1);
}



void test_queue_init(void* args){
  disastrOS_printStatus();
  printf("Starting queue tests\n");
  disastrOS_spawn(sleeperFunction,0);

  // Test switch
  int test_n = 1;
  int alive_children;

  switch (test_n){
  case 0:
    printf("Test0: spawning 2 threads, that open the same queue and close it after some time\n");
    disastrOS_spawn(test_queue_child_0_0, 0);
    disastrOS_spawn(test_queue_child_0_1, 0);

    alive_children = 2;
    break;
  
  case 1:
    printf("Test1: spawning 1 threads, that opens a queue, leaves a message and closes it\n");
    disastrOS_spawn(test_queue_child_1_0, 0);

    alive_children = 1;
    break;

  default:
    break;
  }
  
  disastrOS_printStatus();


  int pid;
  int retval;
  while(alive_children>0 && (pid=disastrOS_wait(0, &retval))>=0){ 
    disastrOS_printStatus();
    printf("test_queue_init, child: %d terminated, retval:%d, alive: %d \n",
	   pid, retval, alive_children);
    --alive_children;
  }

  // aftertests
  switch (test_n){
  case 0:
    // TODO 
    // dmq_unlink(0);
    break;
  case 1:
    printf("Test 1: spawning another child that opens the previous queue, reads and prints the message and exits");
    disastrOS_spawn(test_queue_child_1_1,0);
    disastrOS_wait(0,&retval);
    break;

  default:

    break;
  }

  printf("Shutdown\n");
  disastrOS_shutdown();

}

void initFunction(void* args) {
  // disastrOS_printStatus();
  // printf("hello, I am init and I just started\n");
  // disastrOS_spawn(sleeperFunction, 0);
  

  // printf("I feel like to spawn 10 nice threads\n");
  // int alive_children=0;
  // for (int i=0; i<10; ++i) {
  //   int type=0;
  //   int mode=DSOS_CREATE;
  //   printf("mode: %d\n", mode);
  //   printf("opening resource (and creating if necessary)\n");
  //   int fd=disastrOS_openResource(i,type,mode);
  //   printf("fd=%d\n", fd);
  //   disastrOS_spawn(childFunction, 0);
  //   alive_children++;
  // }

  // disastrOS_printStatus();
  // int retval;
  // int pid;
  // while(alive_children>0 && (pid=disastrOS_wait(0, &retval))>=0){ 
  //   disastrOS_printStatus();
  //   printf("initFunction, child: %d terminated, retval:%d, alive: %d \n",
	//    pid, retval, alive_children);
  //   --alive_children;
  // }
  // printf("shutdown!");
  // disastrOS_shutdown();
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
  // disastrOS_start(initFunction, 0, logfilename);
  return 0;
}
