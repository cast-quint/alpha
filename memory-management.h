/*
 * Dimitrios Koropoulis, 3967
 * CS340 - Spring 2021
 * memory-management.h
 */
#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H

#include <stdio.h>

void  mem_cleanup(void);
void* mymalloc(size_t size);
char* mystrdup(const char* s);
FILE* myfopen(const char* pathname, const char* mode);

#endif /* MEMORY_MANAGEMENT_H */