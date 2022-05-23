/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * avm_inst.h
 */


#ifndef AVM_INST_H
#define AVM_INST_H

#include "avm_util.h"


void avm_assign(Memcell* lv, Memcell* rv);
int avm_tobool(Memcell* c);
unsigned int avm_get_env_val(unsigned int i);
void avm_decrement_top(void);
void avm_push_env_val(unsigned int val);
void avm_save_call_env(void);

void execute_assign(Instruction* inst);
void execute_arithm(Instruction* inst);
void execute_jeq(Instruction* inst);
void execute_jne(Instruction* inst);
void execute_relop(Instruction* inst);
void execute_jump(Instruction* inst);
void execute_enterfunc(Instruction* inst);
void execute_exitfunc(Instruction* inst);
void execute_callfunc(Instruction* inst);
void execute_pusharg(Instruction* inst);
void execute_nop(Instruction* inst);

void execute_newtable(Instruction* inst);
void execute_tablegetelem(Instruction* inst);
void execute_tablesetelem(Instruction* inst);

#endif /* AVM_INST_H */