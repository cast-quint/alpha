/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * avm_table.h
 */

#ifndef AVM_TABLE_H
#define AVM_TABLE_H

#define AVM_VALID_TYPES_COUNT   6
#define AVM_HASHTABLE_SIZE      211

typedef struct Memcell Memcell;

typedef struct TableBucket {
    Memcell* key;
    Memcell* value;
    struct TableBucket* next;
} TableBucket;

typedef struct Table {
    unsigned int ref_count;
    unsigned int hash_id;   /* used for hashing the table in another table */
    TableBucket* indexed[AVM_VALID_TYPES_COUNT][AVM_HASHTABLE_SIZE];
    unsigned int indexed_total[AVM_VALID_TYPES_COUNT];
} Table;


extern unsigned int table_count;

Table* avm_new_table(void);
void avm_increment_ref(Table* table);
void avm_decrement_ref(Table* table);
void avm_tablesetelem(Table* table, Memcell* key, Memcell* new_value);

#endif /* AVM_TABLE_H */