/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * avm_lib.h
 */

#ifndef AVM_LIB_H
#define AVM_LIB_H

#include "avm_util.h"

typedef void (*libfunc_ptr)(void);

unsigned int avm_total_args(void);
Memcell* avm_get_arg(unsigned int i);
void avm_call_lib(const char* name);


#endif /* AVM_LIB_H */