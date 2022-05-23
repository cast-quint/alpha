/*
 * Dimitrios Koropoulis, 3967
 * CS340 - Spring 2021
 * symtable.c
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "memory-management.h"
#include "symtable.h"
#include "util.h"

#define FNV_SEED 			16777619
#define FNV_PRIME 			2166136261

#define HEADER_DELIM    "---------------------------------------------"
#define PRINT_DELIM     fprintf(g_symout, "%s\n", HEADER_DELIM)


FILE* g_symout;
extern const char* g_input_filename; /* the input file name, if specified */
extern SymTable* g_symtable;
extern SymTableBucket* g_scope_heads[MAX_SCOPE_COUNT];
extern Expression* g_dummy;
extern Expression* g_current_function;

/* recursive byte-wise hash function using Fowler–Noll–Vo algorithm */
static unsigned int fnv1a(const char* pcKey, unsigned int hash) {

	if (*pcKey == '\0') {
		return hash;
	}

	hash = (*pcKey++ ^ hash) * FNV_PRIME;

	return fnv1a(pcKey, hash);
}

/* wrapper for the hash function */
static unsigned int hash(const char* key) {

	return fnv1a(key, FNV_SEED) % SYM_TABLE_SIZE;
}

char* symtable_get_symbol_type(symbol_type type) {

	switch (type) {
		case S_LOCAL: {
			return "LOCAL";
			break;
		}

		case S_GLOBAL: {
			return "GLOBAL";
			break;
		}

		case S_FORMAL: {
			return "FORMAL";
			break;
		}

		case S_LIBFUNC: {
			return "LIBFUNC";
			break;
		}

		case S_USERFUNC: {
			return "USERFUNC";
			break;
		}

		default: {
			assert(0);
		}
	}
}

static void print_scope_list(SymTableBucket *head) {

	unsigned int scope;

	assert(head);

	scope = head->entry->scope;
	fprintf(g_symout, "\n\n========== SCOPE #%u ==========\n\n", scope);

	while (head != NULL) {
		if (strcmp(head->entry->name, "$tD") == 0 || strcmp(head->entry->name, "$fD") == 0) {
			head = head->next_in_scope;
			continue;
		}

		if (head->entry->type == S_USERFUNC || head->entry->type == S_LIBFUNC) {
			fprintf(g_symout, "Symbol: \"%s\"\n    Type: %s\n    Scope:       %d\n    Line:        %u\n    Local Count: %u\n    IAddress:    %u\n\n", \
			head->entry->name, symtable_get_symbol_type(head->entry->type), \
			head->entry->scope, head->entry->line, head->entry->local_count, head->entry->iaddress);
		} else {
			fprintf(g_symout, "Symbol: \"%s\"\n    Type: %s\n    Scope:  %d\n    Offset: %u\n    Line:   %u\n\n", \
				head->entry->name, symtable_get_symbol_type(head->entry->type), \
				head->entry->scope, head->entry->offset, head->entry->line);
		}

		head = head->next_in_scope;
	}
}

void print_symtable(void) {

	int i = 0;

	PRINT_DELIM;
	fprintf(g_symout, "brief: symtable\ninput: %s\n", g_input_filename);
	PRINT_DELIM;

	assert(g_symtable);
	while (g_scope_heads[i] != NULL) {
		print_scope_list(g_scope_heads[i++]);
	}
}

void symtable_create(void) {

	int i;

	g_symtable = mymalloc(sizeof(SymTable));
	g_symtable->size = 0;
	for (i = 0; i < SYM_TABLE_SIZE; i++) {
		g_symtable->buckets[i] = NULL;
	}

	g_dummy->symbol = symtable_insert("$tD", S_GLOBAL);
	g_current_function->symbol = symtable_insert("$tF", S_USERFUNC);
	g_current_function->symbol->space = SC_PROGRAM;
	g_current_function->symbol->scope = 0;
}

static SymTableBucket* create_bucket(const char* name, symbol_type type) {

	SymTableBucket *new_bucket = NULL;
	SymTableEntry *new_entry = NULL;

	assert(g_symtable);
	assert(name);

	new_entry = mymalloc(sizeof(SymTableEntry));
	new_entry->type = type;
	new_entry->name = mystrdup(name);
	new_entry->space = current_scopespace();
	new_entry->offset = 0;
	new_entry->scope = current_scope();
	new_entry->line = current_line();
	new_entry->local_count = 0;
	new_entry->iaddress = 0;
	new_entry->is_active = TRUE;

	new_bucket = mymalloc(sizeof(SymTableBucket));
	new_bucket->entry = new_entry;
	new_bucket->next = NULL;
	new_bucket->next_in_scope = NULL;

	return new_bucket;
}

SymTableEntry* symtable_insert(const char* name, symbol_type type) {

	SymTableBucket *new_bucket = NULL;
	SymTableBucket *current = NULL;

	unsigned int table_index = 0;

	assert(g_symtable);
	assert(name);

	new_bucket = create_bucket(name, type);
	table_index = hash(name);

	if (g_symtable->buckets[table_index] == NULL) {
		g_symtable->buckets[table_index] = new_bucket;
	} else {
		current = g_symtable->buckets[table_index];
		while (current->next != NULL) {
			current = current->next;
		}

		current->next = new_bucket;
	}

	/* insert into scope list */
	if (g_scope_heads[current_scope()] == NULL) {
		g_scope_heads[current_scope()] = new_bucket;
	} else {
		current = g_scope_heads[current_scope()];
		while (current->next_in_scope != NULL) {
			current = current->next_in_scope;
		}

		current->next_in_scope = new_bucket;
	}

	g_symtable->size++;

	return new_bucket->entry;
}

void symtable_hide(unsigned int scope) {

	SymTableBucket *current = NULL;

	assert(g_symtable);

	current = g_scope_heads[current_scope()];
	while (current != NULL) {
		current->entry->is_active = FALSE;
		current = current->next_in_scope;
	}
}

SymTableEntry* symtable_lookup(const char* name) {

	SymTableBucket *current_bucket = NULL;
	int i;

	assert(g_symtable);
	assert(name);

	for (i = current_scope(); i >= 0; --i) {
		current_bucket = g_scope_heads[i];
		while (current_bucket != NULL) {
			if ((strcmp(current_bucket->entry->name, name) == 0) && (current_bucket->entry->is_active == TRUE)) {
				return current_bucket->entry;
			}
			current_bucket = current_bucket->next_in_scope;
		}
	}

	return NULL;
}

SymTableEntry* symtable_lookup_in_scope(const char* name, unsigned int scope) {

	SymTableBucket *current_bucket = NULL;

	assert(g_symtable);
	assert(name);

	current_bucket = g_scope_heads[scope];
	while (current_bucket != NULL) {
		if ((strcmp(current_bucket->entry->name, name) == 0) && (current_bucket->entry->is_active == TRUE)) {
			return current_bucket->entry;
		}
		current_bucket = current_bucket->next_in_scope;
	}

	return NULL;
}
