#ifndef LINBER_RBTREE
#define LINBER_RBTREE

#include <linux/list.h>
#include <linux/rbtree.h>
#include "linber_types.h"
#include "linber_list.h"

struct RBtree_node {
	struct rb_node node;		// used to manage the node inside the Tree ordered by deadline
	struct list_head RequestHead;
	unsigned long unique_id;
};

int Insert_ReqNode_Sorted_by_Deadline(struct rb_root *root, struct RequestNode *req_node){
	struct RBtree_node *new_container;
	struct rb_node **new = &(root->rb_node), *parent = NULL;
	long int result;
	bool is_unique = true;

	while (*new) {
		struct RBtree_node *this = container_of(*new, struct RBtree_node, node);
		result = (req_node->abs_deadline_ns - this->unique_id);
		parent = *new;
		if (result < 0) {
			new = &((*new)->rb_left);
		} else if (result > 0) {
			new = &((*new)->rb_right);
		} else {	// not unique
			is_unique = false;
			list_add_tail(&req_node->list, &this->RequestHead);
			printk(KERN_INFO "Insert %lu to existing rbtree node\n", req_node->abs_deadline_ns);
			break;
		}
	}

	if(is_unique){
		printk(KERN_INFO "New unique rbtree node creation %lu \n", req_node->abs_deadline_ns);
		new_container = kmalloc(sizeof(struct RBtree_node), GFP_KERNEL);
		if(new_container == NULL){
			printk(KERN_INFO "allocation error\n");
		}
		INIT_LIST_HEAD(&new_container->RequestHead);
		new_container->unique_id = req_node->abs_deadline_ns;
		list_add_tail(&req_node->list, &new_container->RequestHead);

		rb_link_node(&new_container->node, parent, new);
		rb_insert_color(&new_container->node, root);
	}
	return 0;
}

struct RequestNode* get_first_request(struct rb_root *tree){
	struct RequestNode *req_node = NULL;
	struct RBtree_node *node_container = NULL;
	struct rb_node *node = rb_first(tree);
	if(node != NULL){
		node_container = container_of(node, struct RBtree_node, node);
		if(node_container != NULL){
			req_node = Dequeue_Request(&node_container->RequestHead);
			if(list_empty(&node_container->RequestHead)){
				rb_erase(node, tree);
				kfree(node_container);
			}
			if(req_node != NULL){
				printk(KERN_INFO "Removed %lu\n", req_node->abs_deadline_ns);
			}
		}
	}
	return req_node;
}


#endif
