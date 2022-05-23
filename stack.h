/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * stack.c
 */

#ifndef STACK_H
#define STACK_H

#define MAX_STACK_SIZE 512

#include <stdlib.h>

typedef struct Expression Expression;

typedef struct Stack {
	unsigned int items[MAX_STACK_SIZE];
	size_t top;
} Stack;

typedef struct EStack {
    Expression* items[MAX_STACK_SIZE];
    size_t top;
} EStack;

Stack* new_stack(void);
int stack_is_empty(Stack* stack);
unsigned int stack_get_size(Stack* stack);
void push(Stack* stack, unsigned int item);
unsigned int pop(Stack* stack);
unsigned int peek(Stack* stack);

EStack* new_estack(void);
int estack_is_empty(EStack* stack);
unsigned int estack_get_size(EStack* stack);
void epush(EStack* stack, Expression* item);
Expression* epop(EStack* stack);
Expression* epeek(EStack* stack);

#endif /* STACK_H */