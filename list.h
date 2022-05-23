/*
 * Dimitrios Koropoulis, 3967
 * CS340 - Spring 2021
 * list.h
 */

#ifndef LIST_H
#define LIST_H

typedef struct ListNode {
	unsigned int val;
	struct ListNode* next;
} ListNode;

ListNode* list_merge(ListNode* head1, ListNode* head2);
ListNode* list_insert(ListNode* head, unsigned int val);
ListNode* list_make(unsigned int val);

void       list_print(ListNode* head);

#endif /* LIST_H */