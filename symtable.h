/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * symtable.h
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdlib.h>

#define SYM_TABLE_SIZE 		4093


typedef enum {
	SC_PROGRAM,
	SC_LOCAL,
	SC_FORMAL
} scopespace_type;

typedef enum {
	S_GLOBAL,
	S_LOCAL,
	S_FORMAL,
	S_USERFUNC,
	S_LIBFUNC,
} symbol_type;

typedef struct FormalList {
	size_t size;
	struct SymTableEntry **list;
} FormalList;

typedef struct SymTableEntry {
	symbol_type type;			/* symbol type 					  		  			*/
	const char *name;			/* symbol name 					  		  			*/
	scopespace_type space;		/* first declared scope-space 	  		  			*/
	unsigned int offset;		/* if var, relative offset in said scope-space    	*/
	unsigned int scope;			/* general scope in program			     	 		*/
	unsigned int line;			/* first declared line			     	  			*/
	unsigned int local_count;   /* if function, its local variable count  			*/
	unsigned int iaddress;		/* if function, quad number        		  			*/
	int is_active;				/* pretty self explanatory                			*/

} SymTableEntry;

typedef struct SymTableBucket {
	SymTableEntry *entry;
	struct SymTableBucket *next;
	struct SymTableBucket *next_in_scope;
} SymTableBucket;

typedef struct SymTable {
	SymTableBucket* buckets[SYM_TABLE_SIZE];
	size_t size;
} SymTable;

SymTableEntry* symtable_lookup(const char* name);
SymTableEntry* symtable_lookup_in_scope(const char* name, unsigned int scope);
SymTableEntry* symtable_insert(const char* name, symbol_type type);
char* symtable_get_symbol_type(symbol_type type);
void symtable_hide(unsigned int scope);
void symtable_create(void);
void print_symtable(void);
void symtable_free(void);

#endif /* SYMTABLE_H */
