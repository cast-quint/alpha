/*
 * Dimitrios Koropoulis, 3967
 * CS340 - Spring 2021
 * memory-management.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "memory-management.h"

#define EXPAND_SIZE             1024
#define CURR_MEMTABLE_SIZE      (g_memtable_size * sizeof(void *))
#define NEW_MEMTABLE_SIZE       (CURR_MEMTABLE_SIZE + EXPAND_SIZE * sizeof(void *))

static void** g_memtable = NULL;
static unsigned int g_memtable_size = 0;
static unsigned int g_memtable_length = 0;

static void print_error(const char* msg) {

    fprintf(stderr, "\033[1m\033[31merror\033[0m: %s\n", msg);
}

static void memtable_expand(void) {

	void** new_memtable = NULL;

	assert(g_memtable_length == g_memtable_size);

    new_memtable = malloc(NEW_MEMTABLE_SIZE);
    if (g_memtable != NULL) {
        memcpy(new_memtable, g_memtable, CURR_MEMTABLE_SIZE);
        free(g_memtable);
    }

    g_memtable = new_memtable;
    g_memtable_size += EXPAND_SIZE;
}

static void memtable_insert(void* pointer) {

	assert(pointer);

	if (g_memtable_length == g_memtable_size) {
		memtable_expand();
	}

	g_memtable[g_memtable_length++] = pointer;
}

void mem_cleanup(void) {

	size_t i;

	for (i = 0; i < g_memtable_length; i++) {
		free(g_memtable[i]);
	}

    g_memtable_length = 0;
    g_memtable_size = 0;

	free(g_memtable);
    g_memtable = NULL;
}

void* mymalloc(size_t size) {

    void* ptr = malloc(size);

	if (ptr == NULL) {
        perror("mymalloc:");
        exit(EXIT_FAILURE);
    }

    memtable_insert(ptr);

    return ptr;
}

char* mystrdup(const char* s) {

	char* new_string_ptr = NULL;

	new_string_ptr = strdup(s);
	memtable_insert(new_string_ptr);

	return new_string_ptr;
}

FILE* myfopen(const char* pathname, const char* mode) {

    FILE* fptr = NULL;
    char buffer[512];

    fptr = fopen(pathname, mode);

    if (fptr == NULL) {
        snprintf(buffer, 512, "%s does not exist.", pathname);
        print_error(buffer);
        return NULL;
    }

    return fptr;
}

