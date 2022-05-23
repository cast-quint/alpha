/*
 * Dimitrios Koropoulis, 3967
 * CS340 - Spring 2021
 * list.c
 */


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "memory-management.h"
#include "list.h"


static ListNode* list_new_node(unsigned int val) {

	ListNode* node = NULL;

	node = mymalloc(sizeof(ListNode));
	node->val = val;
	node->next = NULL;

	return node;
}

ListNode* list_insert(ListNode* head, unsigned int val) {

	ListNode* newnode = NULL;

	assert(head);

	newnode = list_new_node(val);

	newnode->next = head;
	head = newnode;

	return head;
}

ListNode* list_make(unsigned int val) {
	return list_new_node(val);
}


ListNode* list_merge(ListNode* head1, ListNode* head2) {

	ListNode* new_head = NULL;

	if (head1 != NULL) {
		new_head = list_make(head1->val);
	} else if (head2 != NULL) {
		new_head = list_make(head2->val);
	} else {
		return NULL;
	}

	if (head1 != NULL) {
		head1 = head1->next;
		while (head1 != NULL) {
			new_head = list_insert(new_head, head1->val);
			head1 = head1->next;
		}
	}

	if (head2 != NULL) {
		while (head2 != NULL) {
			new_head = list_insert(new_head, head2->val);
			head2 = head2->next;
		}
	}

	return new_head;
}

void list_print(ListNode* head) {

	while (head != NULL) {
		printf("|%u|->", head->val);
		head = head->next;
	}

	puts("(null)");
}

