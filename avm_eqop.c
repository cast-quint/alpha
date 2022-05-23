/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * avm_eqop.c
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "tcode.h"
#include "avm_inst.h"


static int number_to_bool(Memcell* cell){

    assert(cell->type == M_NUMBER);

    return cell->val_num != 0;
}

static int string_to_bool(Memcell* cell){

    assert(cell->type == M_STRING);

    return strlen(cell->val_string) != 0;
}

static int bool_to_bool(Memcell* cell){

    assert(cell->type == M_BOOL);

    return cell->val_bool;
}

static int table_to_bool(Memcell* cell){

    assert(cell->type == M_TABLE);

    return 1;
}

static int userfunc_to_bool(Memcell* cell){

    assert(cell->type == M_USERFUNC);

    return 1;
}

static int libfunc_to_bool(Memcell* cell){

    assert(cell->type == M_LIBFUNC);

    return 1;
}

static int nil_to_bool(Memcell* cell){

    assert(cell->type == M_NIL);

    return 0;
}

static int undef_to_bool(Memcell* cell){

    assert(cell->type == M_UNDEF);

    assert(0);
}

static int compare_number(Memcell* rv1, Memcell* rv2, int mode) {

    assert(rv1->type == M_NUMBER);
    assert(rv2->type == M_NUMBER);

    switch (mode) {

        case 0: {
            return rv1->val_num == rv2->val_num;
        }

        case 1: {
            return rv1->val_num != rv2->val_num;

        }

        default: {
            assert(0);
        }
    }
}

static int compare_string(Memcell* rv1, Memcell* rv2, int mode) {

    assert(rv1->type == M_STRING);
    assert(rv2->type == M_STRING);
    switch (mode) {

        case 0: {
            return strcmp(rv1->val_string, rv2->val_string) == 0;

        }

        case 1: {
            return strcmp(rv1->val_string, rv2->val_string) != 0;
        }

        default: {
            assert(0);
        }
    }
}

static int compare_bool(Memcell* rv1, Memcell* rv2, int mode) {

    assert(rv1->type == M_BOOL);
    assert(rv2->type == M_BOOL);
    switch (mode) {

        case 0: {
            return rv1->val_bool == rv2->val_bool;
        }

        case 1: {
            return rv1->val_bool != rv2->val_bool;
        }

        default: {
            assert(0);
        }
    }
}

static int compare_table(Memcell* rv1, Memcell* rv2, int mode) {

    assert(rv1->type == M_TABLE);
    assert(rv2->type == M_TABLE);
    switch (mode) {

        case 0: {
            return rv1->val_table == rv2->val_table;
        }

        case 1: {
            return rv1->val_table != rv2->val_table;

        }

        default: {
            assert(0);
        }
    }
}

static int compare_userfunc(Memcell* rv1, Memcell* rv2, int mode) {

    assert(rv1->type == M_USERFUNC);
    assert(rv2->type == M_USERFUNC);
    switch (mode) {

        case 0: {
            return rv1->val_func == rv2->val_func;
        }

        case 1: {
            return rv1->val_func != rv2->val_func;
        }

        default: {
            assert(0);
        }
    }
}

static int compare_libfunc(Memcell* rv1, Memcell* rv2, int mode) {

    assert(rv1->type == M_LIBFUNC);
    assert(rv2->type == M_LIBFUNC);

    switch (mode) {

        case 0: {
            return strcmp(rv1->val_lib, rv2->val_lib) == 0;
        }

        case 1: {
            return strcmp(rv1->val_lib, rv2->val_lib) != 0;
        }

        default: {
            assert(0);
        }
    }
}

static int (*autobool[])(Memcell *) = {
    number_to_bool,
    string_to_bool,
    bool_to_bool,
    table_to_bool,
    userfunc_to_bool,
    libfunc_to_bool,
    nil_to_bool,
    undef_to_bool
};

static int (*autocomp[])(Memcell *, Memcell*, int) = {
    compare_number,
    compare_string,
    compare_bool,
    compare_table,
    compare_userfunc,
    compare_libfunc
};

int avm_tobool(Memcell* c) {

    assert(c->type >= 0 && c->type < M_UNDEF);

    return autobool[c->type](c);
}

void execute_jeq(Instruction* inst) {


    char buff[512];
    int result = 0;
    Memcell* rv1 = avm_translate_to_memcell(inst->arg1, eax);
    Memcell* rv2 = avm_translate_to_memcell(inst->arg2, ebx);

    assert(inst->result->type == A_LABEL);

    if (rv1->type == M_UNDEF || rv2->type == M_UNDEF) {
        snprintf(buff, 512, "invalid operands in equality (have \"%s\" and \"%s\")", avm_print_type(rv1), avm_print_type(rv2));
        avm_error(buff);

        return;
    }

    if (rv1->type == M_NIL || rv2->type == M_NIL) {
        result = (rv1->type == M_NIL) && (rv2->type == M_NIL);
    } else if (rv1->type == M_BOOL || rv2->type == M_BOOL) {
        result = (avm_tobool(rv1) == avm_tobool(rv2));
    } else if (rv1->type == rv2->type) {
        result = autocomp[rv1->type](rv1, rv2, 0);
    } else {
        snprintf(buff, 512, "invalid operands in equality (have \"%s\" and \"%s\")", avm_print_type(rv1), avm_print_type(rv2));
        avm_error(buff);

        return;
    }


    if (result) {
        pc = inst->result->val;
    }
}

void execute_jne(Instruction* inst) {

    char buff[512];
    int result = 0;
    Memcell* rv1 = avm_translate_to_memcell(inst->arg1, eax);
    Memcell* rv2 = avm_translate_to_memcell(inst->arg2, ebx);

    assert(inst->result->type == A_LABEL);

    if (rv1->type == M_UNDEF || rv2->type == M_UNDEF) {
        snprintf(buff, 512, "invalid operands in inequality (have \"%s\" and \"%s\")", avm_print_type(rv1), avm_print_type(rv2));
        avm_error(buff);

        return;
    }

    if (rv1->type == M_NIL || rv2->type == M_NIL) {
        result = !((rv1->type == M_NIL) && (rv2->type == M_NIL));
    } else if (rv1->type == M_BOOL || rv2->type == M_BOOL) {
        result = (avm_tobool(rv1) != avm_tobool(rv2));
    } else if (rv1->type == rv2->type) {
        result = autocomp[rv1->type](rv1, rv2, 1);
    } else {
        snprintf(buff, 512, "invalid operands in inequality (have \"%s\" and \"%s\")", avm_print_type(rv1), avm_print_type(rv2));
        avm_error(buff);

        return;
    }


    if (result) {
        pc = inst->result->val;
    }
}
