#ifndef LINBER_RBTREE
#define LINBER_RBTREE

#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include "linber_types.h"

//#define DEBUG_RBTREE

struct RBtree_RequestNode {
	struct rb_node node;			// used to manage the node inside the root ordered by deadline
	struct list_head RequestHead;	// queue of request, more request can have the same deadline
	unsigned long unique_id;
};

struct RBtree_ServiceNode {
	struct rb_node node;			// used to manage the node inside the root ordered by uri string
	struct ServiceNode *Service;
	char *service_uri;
};


/*----------------------------------------------------------------
	Insert a ServiceNode in RBtree root, the tree is organized using the service uri.
	If a service with the same uri exists in the tree then return false
	Excute with mutex locked to avoid conflict
----------------------------------------------------------------*/
bool Insert_ServiceNode(struct rb_root *root, struct ServiceNode *ser_node);

/*----------------------------------------------------------------
	Find the ServiceNode corresponding to the passed uri string.
	If there isn't then return NULL.
----------------------------------------------------------------*/
struct ServiceNode *get_ServiceNode(struct rb_root *root, char *service_uri);

/*----------------------------------------------------------------
	Remove and return the ServiceNode corresponding to the passed uri string.
	If there isn't then return NULL.
	Excute with mutex locked to avoid conflict.
----------------------------------------------------------------*/
struct ServiceNode* Remove_Service(struct rb_root *root, char *service_uri);

/*----------------------------------------------------------------
	Remove the first ServiceNode from the tree.
	If the tree is empty then return NULL.
	Excute with mutex locked to avoid conflict.
----------------------------------------------------------------*/
struct ServiceNode* Remove_first_Service(struct rb_root *root);

/*----------------------------------------------------------------
	Insert a RequestNode in RBtree root, the tree is organized using the request abs_deadline_ns
	If is a new node of the tree then create a list of NodeRequest for that node and insert the RequestNode
	If isn't a new node of the tree then add the RequestNode in the node's request queue (more request can have the same deadline)
	Excute with mutex locked to avoid conflict
----------------------------------------------------------------*/
bool Insert_ReqNode_Sorted_by_Deadline(struct rb_root *root, struct RequestNode *req_node);

/*----------------------------------------------------------------
	Remove the first request(FIFO order) from the queue of the first node, that is the node with shortest deadline
	Excute with mutex locked to avoid conflict
----------------------------------------------------------------*/
struct RequestNode* Remove_first_request(struct rb_root *root);

/*----------------------------------------------------------------
	Find the RequestNode in the tree, corresponding to the passed deadline (used to navigate the tree)
	and corresponding to the passed token (used to find the request in the node's request queue)
	Excute with mutex locked to avoid conflict
----------------------------------------------------------------*/
struct RequestNode* RBtree_get_request(struct rb_root *root, unsigned long abs_deadline_ns, unsigned long token);

#endif
