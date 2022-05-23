/*
 * Dimitrios Koropoulis, 3967
 * CS340 - Spring 2021
 * stack.c
 */
#include <assert.h>
#include <stdlib.h>

#include "memory-management.h"
#include "util.h"
#include "stack.h"

Stack* new_stack(void) {

	Stack* stack = NULL;

	stack = mymalloc(sizeof(Stack));
	stack->top = 0;

	return stack;
}

int stack_is_empty(Stack* stack) {

	assert(stack);

	return !stack->top;
}

unsigned int stack_get_size(Stack* stack) {

	assert(stack);

	return stack->top;
}

void push(Stack* stack, unsigned int item) {

	assert(stack);
	assert(stack_get_size(stack) <= MAX_STACK_SIZE);

	stack->items[stack->top++] = item;
}

unsigned int pop(Stack* stack) {

	assert(stack);
	assert(stack_get_size(stack) > 0);

	return stack->items[--stack->top];
}

unsigned int peek(Stack* stack) {

	assert(stack);
	assert(stack_get_size(stack) > 0);

	return stack->items[stack->top - 1];
}

EStack* new_estack(void) {

	EStack* stack = NULL;

	stack = mymalloc(sizeof(EStack));
	stack->top = 0;

	return stack;
}

int estack_is_empty(EStack* stack) {

	assert(stack);

	return !stack->top;
}

unsigned int estack_get_size(EStack* stack) {

	assert(stack);

	return stack->top;
}

void epush(EStack* stack, Expression* item) {

	assert(stack);
	assert(estack_get_size(stack) <= MAX_STACK_SIZE);

	stack->items[stack->top++] = item;
}

Expression* epop(EStack* stack) {

	assert(stack);
	assert(estack_get_size(stack) > 0);

	return stack->items[--stack->top];
}

Expression* epeek(EStack* stack) {

	assert(stack);
	assert(estack_get_size(stack) > 0);

	return stack->items[stack->top - 1];
}