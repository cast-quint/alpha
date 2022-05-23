/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * avm_func.c
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "avm_lib.h"
#include "avm_inst.h"
#include "tcode.h"


unsigned int total_actual_args = 0;

unsigned int avm_get_env_val(unsigned int i) {

    double value;

    assert(i >= 0 && i < AVM_STACK_SIZE);

    assert(avm_stack[i].type == M_NUMBER);

    value = avm_stack[i].val_num;

    assert(avm_stack[i].val_num == value);

    return (unsigned int)value;
}

void avm_decrement_top(void) {

    if (top == 0) {
        avm_error("stack overflow");
        execution_done = 1;

        return;
    }

    --top;
}

void avm_push_env_val(unsigned int val) {

    avm_stack[top].type = M_NUMBER;
    avm_stack[top].val_num = val;
    avm_decrement_top();
}

void avm_save_call_env(void) {

    avm_push_env_val(total_actual_args);
    avm_push_env_val(pc + 1);
    avm_push_env_val(top + total_actual_args + 2);
    avm_push_env_val(topsp);
}

void execute_pusharg(Instruction* inst) {

    Memcell* arg = avm_translate_to_memcell(inst->result, eax);

    avm_assign(&avm_stack[top], arg);
    avm_decrement_top();

    ++total_actual_args;
}

void execute_callfunc(Instruction* inst) {

    Memcell* func = avm_translate_to_memcell(inst->result, eax);
    char buff[1024];

    assert(func);

    avm_save_call_env();

    switch (func->type) {

        case M_USERFUNC: {

            pc = func->val_func;

            assert(pc < AVM_PC_ENDING);
            assert(avm_instructions[pc]->op == VOP_ENTERFUNC);

            break;
        }

        case M_STRING: {
            avm_call_lib(func->val_string);
            break;
        }

        case M_LIBFUNC: {
            avm_call_lib(func->val_lib);
            break;
        }

        default: {
            snprintf(buff, 1024, "call: cannot bind %s to function", avm_tostring(func));
            execution_done = 1;
            break;
        }
    }
}

void execute_enterfunc(Instruction* inst) {

    Memcell* func = avm_translate_to_memcell(inst->result, eax);

    assert(func);
    assert(pc == func->val_func);

    total_actual_args = 0;
    topsp = top;
    top = top - avm_get_func(inst->result->val)->local_size;
}

void execute_exitfunc(Instruction* inst) {

    unsigned int top_old = top;

    top = avm_get_env_val(topsp   + AVM_SAVED_TOP_OFFSET);
    pc = avm_get_env_val(topsp    + AVM_SAVED_PC_OFFSET);
    topsp = avm_get_env_val(topsp + AVM_SAVED_TOPSP_OFFSET);

    while (top_old <= top) {
        //printf("clearing stack[%u]...\n", top_old);
        avm_clear(&avm_stack[top_old]);
        ++top_old;
    }
}
