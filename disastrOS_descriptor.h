#pragma once
#include "linked_list.h"
#include "disastrOS_pcb.h"
#include "disastrOS_resource.h"


struct DescriptorPtr;

typedef struct Descriptor{
  ListItem list;
  PCB* pcb;
  Resource* resource;
  int fd;
  // array that holds 3 ListItem elements. In the specific case of queues, those pointers refer to 
  // the entries in queue's readers, writers and non-block lists, but depending on the element  
  // described by the descriptor, this array may have other meanings and not be a waste of space
  ListItem* rwn[3]; 
  struct DescriptorPtr* ptr; // pointer to the entry in the resource list
} Descriptor;

typedef struct DescriptorPtr{
  ListItem list;
  Descriptor* descriptor;
} DescriptorPtr;

void Descriptor_init();
Descriptor* Descriptor_alloc(int fd, Resource* res, PCB* pcb);
int Descriptor_free(Descriptor* d);
Descriptor*  DescriptorList_byFd(ListHead* l, int fd);
void DescriptorList_print(ListHead* l);

DescriptorPtr* DescriptorPtr_alloc(Descriptor* descriptor);
int DescriptorPtr_free(DescriptorPtr* d);
void DescriptorPtrList_print(ListHead* l);
