#ifndef LINBER_RBTREE
#define LINBER_RBTREE

#include <linux/list.h>
#include <linux/rbtree.h>
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
	Extract the first list_node from the list if the list_head,
	Excute with mutex locked to avoid a deletion while another is iterating
-----------------------------------------------------------------*/
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

/*----------------------------------------------------------------
	Insert a ServiceNode in RBtree root, the tree is organized using the service uri.
	If a service with the same uri exists in the tree then return false
	Excute with mutex locked to avoid conflict
----------------------------------------------------------------*/
bool Insert_ServiceNode(struct rb_root *root, struct ServiceNode *ser_node){
	struct RBtree_ServiceNode *new_container, *this;
	struct rb_node **new = &(root->rb_node), *parent = NULL;
	long int result;

	while (*new) {
		this = container_of(*new, struct RBtree_ServiceNode, node);
		result = strcmp(ser_node->uri, this->service_uri);
		parent = *new;
		if (result < 0) {
			new = &((*new)->rb_left);
		} else if (result > 0) {
			new = &((*new)->rb_right);
		} else {	// not unique
			return false;
		}
	}
	new_container = kmalloc(sizeof(struct RBtree_ServiceNode), GFP_KERNEL);
	if(new_container == NULL){
		return false;
	}
	new_container->service_uri = ser_node->uri;				// I don't need to copy the string
	new_container->Service = ser_node;
	rb_link_node(&new_container->node, parent, new);
	rb_insert_color(&new_container->node, root);
	return true;
}

/*----------------------------------------------------------------
	Find the ServiceNode corresponding to the passed uri string.
	If there isn't then return NULL.
----------------------------------------------------------------*/
struct ServiceNode *get_ServiceNode(struct rb_root *root, char *service_uri){
	struct rb_node *node = root->rb_node;
	struct RBtree_ServiceNode *container;
	int result;

	while(node) {
		container = container_of(node, struct RBtree_ServiceNode, node);
		if(container != NULL){
			result = strcmp(service_uri, container->service_uri);
			if (result < 0){
				node = node->rb_left;
			} else if(result > 0){
				node = node->rb_right;
			} else {
				return container->Service;
			}
		}
	}
	return NULL;
}

/*----------------------------------------------------------------
	Remove and return the ServiceNode corresponding to the passed uri string.
	If there isn't then return NULL.
	Excute with mutex locked to avoid conflict.
----------------------------------------------------------------*/
struct ServiceNode* Remove_Service(struct rb_root *root, char *service_uri){
	struct ServiceNode *ser_node = NULL;
	struct rb_node *node = root->rb_node;
	struct RBtree_ServiceNode *container;
	int result;

	while(node) {
		container = container_of(node, struct RBtree_ServiceNode, node);
		if(container != NULL){
			result = strcmp(service_uri, container->service_uri);
			if (result < 0){
				node = node->rb_left;
			} else if(result > 0){
				node = node->rb_right;
			} else {
				ser_node = container->Service;
				rb_erase(node, root);
				kfree(container);
				return ser_node;
			}
		}
	}
	return ser_node;
}

/*----------------------------------------------------------------
	Remove the first ServiceNode from the tree.
	If the tree is empty then return NULL.
	Excute with mutex locked to avoid conflict.
----------------------------------------------------------------*/
struct ServiceNode* Remove_first_Service(struct rb_root *root){
	struct ServiceNode *ser_node = NULL;
	struct RBtree_ServiceNode *container = NULL;
	struct rb_node *node = rb_first(root);
	if(node != NULL){
		container = container_of(node, struct RBtree_ServiceNode, node);
		if(container != NULL){
			ser_node = container->Service;
			rb_erase(node, root);
			kfree(container);
		}
	}
	return ser_node;
}

/*----------------------------------------------------------------
	Insert a RequestNode in RBtree root, the tree is organized using the request abs_deadline_ns
	If is a new node of the tree then create a list of NodeRequest for that node and insert the RequestNode
	If isn't a new node of the tree then add the RequestNode in the node's request queue (more request can have the same deadline)
	Excute with mutex locked to avoid conflict
----------------------------------------------------------------*/
bool Insert_ReqNode_Sorted_by_Deadline(struct rb_root *root, struct RequestNode *req_node){
	struct RBtree_RequestNode *new_container, *this;
	struct rb_node **new = &(root->rb_node), *parent = NULL;
	long int result;
	bool is_unique = true;

	while (*new) {
		this = container_of(*new, struct RBtree_RequestNode, node);
		result = (req_node->abs_deadline_ns - this->unique_id);
		parent = *new;
		if (result < 0) {
			new = &((*new)->rb_left);
		} else if (result > 0) {
			new = &((*new)->rb_right);
		} else {	// not unique
			is_unique = false;
			list_add_tail(&req_node->list, &this->RequestHead);
			#ifdef DEBUG_RBTREE
				printk(KERN_INFO "linber:: Insert %lu to existing rbtree node\n", req_node->abs_deadline_ns);
			#endif
			break;
		}
	}

	if(is_unique){
		#ifdef DEBUG_RBTREE
			printk(KERN_INFO "linber:: New unique rbtree node creation %lu \n", req_node->abs_deadline_ns);
		#endif
		new_container = kmalloc(sizeof(struct RBtree_RequestNode), GFP_KERNEL);
		if(new_container == NULL){
			return false;
		}

		INIT_LIST_HEAD(&new_container->RequestHead);
		new_container->unique_id = req_node->abs_deadline_ns;
		list_add_tail(&req_node->list, &new_container->RequestHead);

		rb_link_node(&new_container->node, parent, new);
		rb_insert_color(&new_container->node, root);
	}
	return true;
}

/*----------------------------------------------------------------
	Remove the first request(FIFO order) from the queue of the first node, that is the node with shortest deadline
	Excute with mutex locked to avoid conflict
----------------------------------------------------------------*/
struct RequestNode* Remove_first_request(struct rb_root *root){
	struct RequestNode *req_node = NULL;
	struct RBtree_RequestNode *node_container = NULL;
	struct rb_node *node = rb_first(root);
	if(node != NULL){
		node_container = container_of(node, struct RBtree_RequestNode, node);
		if(node_container != NULL){
			req_node = Dequeue_Request(&node_container->RequestHead);
			if(list_empty(&node_container->RequestHead)){
				rb_erase(node, root);
				kfree(node_container);
			}
			if(req_node != NULL){
				#ifdef DEBUG_RBTREE
					printk(KERN_INFO "linber:: Removed %lu\n", req_node->abs_deadline_ns);
				#endif
			}
		}
	}
	return req_node;
}

/*----------------------------------------------------------------
	Find the RequestNode in the tree, corresponding to the passed deadline (used to navigate the tree)
	and corresponding to the passed token (used to find the request in the node's request queue)
	Excute with mutex locked to avoid conflict
----------------------------------------------------------------*/
struct RequestNode* RBtree_get_request(struct rb_root *root, unsigned long abs_deadline_ns, unsigned long token){
	struct RequestNode *req_node = NULL;
	struct rb_node *node = root->rb_node;
	struct RBtree_RequestNode *container;
	int result;

	while(node) {
		container = container_of(node, struct RBtree_RequestNode, node);
		if(container != NULL){
			result = (abs_deadline_ns - container->unique_id);
			if (result < 0){
				node = node->rb_left;
			} else if(result > 0){
				node = node->rb_right;
			} else {
				list_for_each_entry(req_node, &container->RequestHead, list){
					if(req_node->token == token){
						return req_node;
					}
				}
			}
		}
	}
	return NULL;
}

#endif
