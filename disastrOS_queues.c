#include "disastrOS_constants.h"
#include "disastrOS_queues.h"
#include "disastrOS_globals.h"

dmqd_t dmq_open(const char *name, int dflag, mode_t mode, struct dmq_attr *attr){
    // create a new resource, with the next id available
    Resource_alloc(++last_resource_id % MAX_NUM_RESOURCES,QUEUE_RES);
    //TODO: what if there are no more resources available?
}