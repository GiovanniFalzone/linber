#ifndef LINBER_LIST
#define LINBER_LIST

#include "linber_types.h"

static RequestNode *Dequeue_Request(struct list_head *head){
	RequestNode *req_node = NULL;
	if(list_empty(head)){
		;
	} else {
		req_node = list_first_entry(head, RequestNode , list);
		list_del(&req_node->list);
	}
	return req_node;
}

#endif