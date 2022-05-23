/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * avm_assign.c
 */

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "tcode.h"
#include "avm_inst.h"
#include "avm_table.h"

void avm_assign(Memcell* lv, Memcell* rv) {

	assert(lv && rv);

	if (lv == rv) {
		return;
	}

	if (lv->type == M_TABLE &&
		rv->type == M_TABLE &&
		lv->val_table == rv->val_table) {
		return;
	}

	if (rv != erx && rv->type == M_UNDEF) {
		avm_warning("assigning undefined value!");
	}

	avm_clear(lv);

	memcpy(lv, rv, sizeof(Memcell));

	/*
	 * since memcpy is shallow
	 * if char* pointer (string), copy the string
	 * if table, increase the reference counter of said table
     */
	if (lv->type == M_STRING) {
		lv->val_string = strdup(rv->val_string);
	} else if (lv->type == M_TABLE) {
		avm_increment_ref(lv->val_table);
	}
}


void execute_assign(Instruction* inst) {

	Memcell* lv = avm_translate_to_memcell(inst->result, NULL);
	Memcell* rv = avm_translate_to_memcell(inst->arg1, eax);

	assert_memcell(lv);
	assert(rv);

	avm_assign(lv, rv);
}

