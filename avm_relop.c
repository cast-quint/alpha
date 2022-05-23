/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * avm_relop.c
 */

#include <stdio.h>
#include <assert.h>
#include "tcode.h"
#include "avm_inst.h"

static int jgt_impl(double x, double y) {

    return x > y;
}

static int jlt_impl(double x, double y) {

    return x < y;
}

static int jge_impl(double x, double y) {

    return x >= y;
}

static int jle_impl(double x, double y) {

    return x <= y;
}

static int (*autorelop[])(double, double) = {
    jgt_impl,
    jlt_impl,
    jge_impl,
    jle_impl
};

void execute_relop(Instruction* inst) {

    char buff[512];
    int cond;

    Memcell* rv1 = avm_translate_to_memcell(inst->arg1, eax);
    Memcell* rv2 = avm_translate_to_memcell(inst->arg2, ebx);

    assert(rv1);
    assert(rv2);

    if (rv1->type != M_NUMBER || rv2->type != M_NUMBER) {
        snprintf(buff, 512, "invalid operands in relational operation (have \"%s\" and \"%s\")", avm_print_type(rv1), avm_print_type(rv2));
        avm_error(buff);

        return;
    }


    cond = autorelop[inst->op - VOP_JGT](rv1->val_num, rv2->val_num);
    if (cond) {
        assert(inst->result->type == A_LABEL);
        pc = inst->result->val;
    }
}

void execute_jump(Instruction* inst) {

    assert(inst->result->type == A_LABEL);
    assert(inst->result->val <= AVM_PC_ENDING);


    //printf("jumping to %u ...\n" , inst->result->val);
    pc = inst->result->val;
}