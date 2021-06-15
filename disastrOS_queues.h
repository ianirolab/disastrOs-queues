#pragma once
#include "disastrOS_resource.h"


// flags used to select modes for dmq_open
// each one corresponds to a power of 2, so that they can be put in or
#define D_RDONLY 0x1
#define D_WRONLY 0x2
#define D_RDWE 0x4
#define D_CLOEXEC 0x8
#define D_CREAT 0x16
#define D_EXCL 0x32
#define D_NONBLOCK 0x64



typedef int dmqd_t;
typedef unsigned int dssize_t;
typedef int dsize_t;

struct dmq_attr {
    long int dmq_flags;		/* Message queue flags */
    long int dmq_maxmsg;	/* Maximum number of messages */
    long int dmq_msgsize;	/* Maximum message size */
    long int dmq_curmsgs;	/* Number of messages currently queued */
};

//TODO understand use of mode_t and use a coherent type
typedef int mode_t;


int dmq_close(dmqd_t mqdes);
// TODO: uncomment and convert sigevent
//int dmq_notify(dmqd_t mqdes, const struct sigevent *sevp);

// TODO: consider converting dflag to char?
dmqd_t dmq_open(const char *name, int dflag, mode_t mode, struct dmq_attr *attr);
dssize_t dmq_receive(dmqd_t mqdes, char *msg_ptr, dsize_t msg_len, unsigned int *msg_prio);
int dmq_send(dmqd_t mqdes, const char *msg_ptr, dsize_t msg_len, unsigned int msg_prio);
int dmq_setattr(dmqd_t mqdes, const struct dmq_attr *newattr, struct dmq_attr *oldattr);
int mq_unlink(const char *name);
