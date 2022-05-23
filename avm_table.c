/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * avm_table.c
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "tcode.h"
#include <string.h>
#include "avm_inst.h"
#include "avm_table.h"

#define FNV_SEED 16777619
#define FNV_PRIME 2166136261
#define NUM_RAND_SEED 836

unsigned int table_count = 0;

static unsigned int fnv1a(const char* pcKey, unsigned int hash) {

    if (*pcKey == '\0') {
        return hash;
    }

    hash = (*pcKey++ ^ hash) * FNV_PRIME;

    return fnv1a(pcKey, hash);
}

static unsigned int hash_num(double num) {

    return ((unsigned int)(num + NUM_RAND_SEED)) % AVM_HASHTABLE_SIZE;
}

static unsigned int hash_str(const char* key) {

    return fnv1a(key, FNV_SEED) % AVM_HASHTABLE_SIZE;
}

static void keynum_setelem(Table* table, Memcell* key, Memcell* new_value) {

    unsigned int i;
    TableBucket* current = NULL;
    TableBucket* prev = NULL;

    assert(table && key && new_value);
    assert(key->type == M_NUMBER);

    i = hash_num(key->val_num);

    current = table->indexed[M_NUMBER][i];
    while (current != NULL && current->value->type != M_NIL && current->key->val_num != key->val_num) {
        prev = current;
        current = current->next;
    }

    if (current && current->value->type != M_NIL) {
        avm_clear(current->value);
    }

    if (new_value->type == M_NIL) {
        return;
    }

    if (!current) {

        current = malloc(sizeof(TableBucket));

        current->key = malloc(sizeof(Memcell));
        current->key->type = M_UNDEF;

        current->value = malloc(sizeof(Memcell));
        current->value->type = M_UNDEF;

        current->next = NULL;

        avm_assign(current->key, key);

        prev ? (prev->next = current) : (table->indexed[M_NUMBER][i] = current);
    }

    avm_assign(current->value, new_value);
    ++(table->indexed_total[M_NUMBER]);
}

static void keystr_setelem(Table* table, Memcell* key, Memcell* new_value) {

    unsigned int i;
    TableBucket* current = NULL;
    TableBucket* prev = NULL;

    assert(table && key && new_value);
    assert(key->type == M_STRING);

    i = hash_str(key->val_string);

    current = table->indexed[M_STRING][i];
    while (current != NULL && current->value->type != M_NIL && strcmp(current->key->val_string, key->val_string) != 0) {
        prev = current;
        current = current->next;
    }

    if (current && current->value->type != M_NIL) {
        avm_clear(current->value);
    }

    if (new_value->type == M_NIL) {
        return;
    }

    if (!current) {

        current = malloc(sizeof(TableBucket));

        current->key = malloc(sizeof(Memcell));
        current->key->type = M_UNDEF;

        current->value = malloc(sizeof(Memcell));
        current->value->type = M_UNDEF;

        current->next = NULL;

        avm_assign(current->key, key);

        prev ? (prev->next = current) : (table->indexed[M_STRING][i] = current);
    }

    avm_assign(current->value, new_value);
    ++(table->indexed_total[M_STRING]);
}

static void keybool_setelem(Table* table, Memcell* key, Memcell* new_value) {

    TableBucket* current = NULL;
    unsigned int i = key->val_bool;

    assert(table && key && new_value);
    assert(key->type == M_BOOL);

    assert(i == 0 || i == 1);

    current = table->indexed[M_BOOL][i];

    if (current && current->value->type != M_NIL) {
        avm_clear(current->value);
    }

    if (new_value->type == M_NIL) {
        return;
    }

    if (!current) {

        current = malloc(sizeof(TableBucket));

        current->key = malloc(sizeof(Memcell));
        current->key->type = M_UNDEF;

        current->value = malloc(sizeof(Memcell));
        current->value->type = M_UNDEF;

        current->next = NULL;

        avm_assign(current->key, key);

        table->indexed[M_BOOL][i] = current;
    }

    avm_assign(current->value, new_value);
    ++(table->indexed_total[M_BOOL]);
}

static void keytable_setelem(Table* table, Memcell* key, Memcell* new_value) {

    unsigned int i;
    TableBucket* current = NULL;
    TableBucket* prev = NULL;

    assert(table && key && new_value);
    assert(key->type == M_TABLE);

    i = hash_num(key->val_table->hash_id);

    current = table->indexed[M_TABLE][i];
    while (current != NULL && current->value->type != M_NIL && current->key->val_table != key->val_table) {
        prev = current;
        current = current->next;
    }

    if (current && current->value->type != M_NIL) {
        avm_clear(current->value);
    }

    if (new_value->type == M_NIL) {
        return;
    }

    if (!current) {

        current = malloc(sizeof(TableBucket));

        current->key = malloc(sizeof(Memcell));
        current->key->type = M_UNDEF;

        current->value = malloc(sizeof(Memcell));
        current->value->type = M_UNDEF;

        current->next = NULL;

        avm_assign(current->key, key);

        prev ? (prev->next = current) : (table->indexed[M_TABLE][i] = current);
    }

    avm_assign(current->value, new_value);
    ++(table->indexed_total[M_TABLE]);
}

static void keyfunc_setelem(Table* table, Memcell* key, Memcell* new_value) {

    unsigned int i;
    TableBucket* current = NULL;
    TableBucket* prev = NULL;

    assert(table && key && new_value);
    assert(key->type == M_USERFUNC);

    i = hash_num(key->val_func);

    current = table->indexed[M_USERFUNC][i];
    while (current != NULL && current->value->type != M_NIL && current->key->val_func != key->val_func) {
        prev = current;
        current = current->next;
    }

    if (current && current->value->type != M_NIL) {
        avm_clear(current->value);
    }

    if (new_value->type == M_NIL) {
        return;
    }

    if (!current) {

        current = malloc(sizeof(TableBucket));

        current->key = malloc(sizeof(Memcell));
        current->key->type = M_UNDEF;

        current->value = malloc(sizeof(Memcell));
        current->value->type = M_UNDEF;

        current->next = NULL;

        avm_assign(current->key, key);

        prev ? (prev->next = current) : (table->indexed[M_USERFUNC][i] = current);
    }

    avm_assign(current->value, new_value);
    ++(table->indexed_total[M_USERFUNC]);
}

static void keylib_setelem(Table* table, Memcell* key, Memcell* new_value) {

    unsigned int i;
    TableBucket* current = NULL;
    TableBucket* prev = NULL;

    assert(table && key && new_value);
    assert(key->type == M_LIBFUNC);

    i = hash_str(key->val_lib);

    current = table->indexed[M_LIBFUNC][i];
    while (current != NULL && current->value->type != M_NIL && strcmp(current->key->val_lib, key->val_lib) != 0) {
        prev = current;
        current = current->next;
    }

    if (current && current->value->type != M_NIL) {
        avm_clear(current->value);
    }

    if (new_value->type == M_NIL) {
        return;
    }

    if (!current) {

        current = malloc(sizeof(TableBucket));

        current->key = malloc(sizeof(Memcell));
        current->key->type = M_UNDEF;

        current->value = malloc(sizeof(Memcell));
        current->value->type = M_UNDEF;

        current->next = NULL;

        avm_assign(current->key, key);

        prev ? (prev->next = current) : (table->indexed[M_LIBFUNC][i] = current);
    }

    avm_assign(current->value, new_value);
    ++(table->indexed_total[M_LIBFUNC]);
}

static void (*autoset[])(Table*, Memcell*, Memcell*) = {
    keynum_setelem,
    keystr_setelem,
    keybool_setelem,
    keytable_setelem,
    keyfunc_setelem,
    keylib_setelem
};

static void avm_destroy_table(Table* table) {

    size_t i;
    memcell_t type;
    TableBucket* current = NULL;
    TableBucket* temp = NULL;

    assert(table);
    assert(table->ref_count == 0);

    for (type = 0; type < AVM_VALID_TYPES_COUNT;  ++type) {

        if (table->indexed_total[type] == 0) {
            continue;
        }

        for (i = 0; i < AVM_HASHTABLE_SIZE; ++i) {
            current = table->indexed[type][i];
            while (current) {
                temp = current;
                current = current->next;

                if (temp->key->type == M_STRING) free(temp->key->val_string);
                free(temp->key);

                avm_clear(temp->value);
                free(temp->value);
                free(temp);

                --(table->indexed_total[type]);
            }
        }
    }

    for (type = 0; type < AVM_VALID_TYPES_COUNT; ++type) {
        assert(table->indexed_total[type] == 0);
    }

    free(table);
}

void avm_increment_ref(Table* table) {

    ++table->ref_count;
}

void avm_decrement_ref(Table* table) {

    assert(table->ref_count > 0);

    if (--(table->ref_count) == 0) {
        avm_destroy_table(table);
    }
}

Table* avm_new_table(void) {

    memcell_t t;
    size_t i;
    Table* table = malloc(sizeof(Table));

    table->hash_id = ++table_count;
    table->ref_count = 0;

    for (t = 0; t < AVM_VALID_TYPES_COUNT; ++t) {
        table->indexed_total[t] = 0;
    }

    for (t = 0; t < AVM_VALID_TYPES_COUNT; ++t) {
        for (i = 0; i < AVM_HASHTABLE_SIZE; ++i) {
            table->indexed[t][i] = NULL;
        }
    }

    return table;
}

void execute_newtable(Instruction* inst) {

    Memcell* lv = avm_translate_to_memcell(inst->result, NULL);

    assert_memcell(lv);

    lv->type = M_TABLE;
    lv->val_table = avm_new_table();
    avm_increment_ref(lv->val_table);
}

static Memcell* avm_tablegetelem(Table* table, Memcell* key) {

    unsigned int i;
    TableBucket* current = NULL;

    assert(table);
    assert(key);
    assert(key->type != M_NIL && key->type != M_UNDEF);

    switch (key->type) {

        case M_NUMBER: {
            i = hash_num(key->val_num);
            current = table->indexed[M_NUMBER][i];
            while (current != NULL && current->key->val_num != key->val_num) {
                current = current->next;
            }

            if (!current) {
                return NULL;
            }

            break;
        }

        case M_STRING: {
            i = hash_str(key->val_string);
            current = table->indexed[M_STRING][i];
            while (current != NULL && strcmp(current->key->val_string, key->val_string) != 0) {
                current = current->next;
            }

            if (!current) {
                return NULL;
            }

            break;
        }

        case M_BOOL: {

            current = table->indexed[M_BOOL][key->val_bool];
            if (!current) {
                return NULL;
            }

            break;
        }

        case M_TABLE: {
            i = hash_num(key->val_table->hash_id);
            current = table->indexed[M_TABLE][i];
            while (current != NULL && current->key->val_table != key->val_table) {
                current = current->next;
            }

            if (!current) {
                return NULL;
            }

            break;
        }

        case M_USERFUNC: {

            i = hash_num(key->val_func);
            current = table->indexed[M_USERFUNC][i];
            while (current != NULL && current->key->val_func != key->val_func) {
                current = current->next;
            }

            if (!current) {
                return NULL;
            }

            break;
        }

        case M_LIBFUNC: {
            i = hash_str(key->val_lib);
            current = table->indexed[M_LIBFUNC][i];
            while (current != NULL && strcmp(current->key->val_lib, key->val_lib) != 0) {
                current = current->next;
            }

            if (!current) {
                return NULL;
            }

            break;
        }

        default: {
            assert(0);
        }
    }


    return current->value;
}

void execute_tablegetelem(Instruction* inst) {

    Memcell* lv = avm_translate_to_memcell(inst->result, NULL);
    Memcell* table = avm_translate_to_memcell(inst->arg1, NULL);
    Memcell* key = avm_translate_to_memcell(inst->arg2, eax);
    Memcell* content = NULL;
    char buff[512];


    assert_memcell(lv);
    assert_memcell(table);
    assert(key);

    if (table->type != M_TABLE) {
        snprintf(buff, 512, "illegal use of %s as table", avm_string_types[table->type]);
        avm_error(buff);
        return;
    }

    if (key->type == M_UNDEF) {
        avm_error("illegal use of an undefined value as table key");
        return;
    }

    if (key->type == M_NIL) {
        avm_error("illegal use of nil as table key");
        return;
    }

    content = avm_tablegetelem(table->val_table, key);

    if (!content) {
        content = malloc(sizeof(Memcell));
        content->type = M_NIL;
        avm_assign(lv, content);
        free(content);
    } else {
        avm_assign(lv, content);
    }
}

void avm_tablesetelem(Table* table, Memcell* key, Memcell* new_value) {

    assert(table && key && new_value);

    autoset[key->type](table, key, new_value);
}

void execute_tablesetelem(Instruction* inst) {

    Memcell* table = avm_translate_to_memcell(inst->result, NULL);
    Memcell* key = avm_translate_to_memcell(inst->arg1, eax);
    Memcell* value = avm_translate_to_memcell(inst->arg2, ebx);
    char buff[128];

    assert_memcell(table);
    assert(key);
    assert(value);

    if (table->type != M_TABLE) {
        snprintf(buff, 128, "illegal use of %s as table", avm_string_types[table->type]);
        avm_error(buff);
        return;
    }

    if (key->type == M_UNDEF) {
        avm_error("illegal use of undefined value as key");
        return;
    }

    if (key->type == M_NIL) {
        avm_error("illegal use of nil as key");
        return;
    }

    avm_tablesetelem(table->val_table, key, value);
}













