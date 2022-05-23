/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * avm_arithm.c
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "tcode.h"
#include "avm_inst.h"


static char* add_str(char* str1, char* str2) {

	char* result = NULL;

	result = malloc((strlen(str1) + strlen(str2) + 1) * sizeof(char));
	strcpy(result, str1);
	strcat(result, str2);

	return result;
}

static double add_impl(double x, double y) {

	return x + y;
}

static double sub_impl(double x, double y) {

	return x - y;
}

static double mul_impl(double x, double y) {

	return x * y;
}

static double div_impl(double x, double y) {

	if (!y) {
		avm_error("division with zero!");
	}

	return x / y;
}

static double mod_impl(double x, double y) {

	return ((int)x % (int)y);
}

static double (*autoarithm[])(double, double) = {
	add_impl,
	sub_impl,
	mul_impl,
	div_impl,
	mod_impl
};

void execute_arithm(Instruction* inst) {

	char buff[512];

	Memcell* lv = avm_translate_to_memcell(inst->result, NULL);
	Memcell* rv1 = avm_translate_to_memcell(inst->arg1, eax);
	Memcell* rv2 = avm_translate_to_memcell(inst->arg2, ebx);

	assert_memcell(lv);
	assert(rv1);
	assert(rv2);

	if (rv1->type == M_STRING &&
		rv2->type == M_STRING &&
		inst->op == VOP_ADD) {

		avm_clear(lv);
		lv->type = M_STRING;
		lv->val_string = add_str(rv1->val_string, rv2->val_string);

		return;
	}

	if (rv1->type != M_NUMBER || rv2->type != M_NUMBER) {
		snprintf(buff, 512, "invalid operands in arithmetic operation (have \"%s\" and \"%s\")", avm_print_type(rv1), avm_print_type(rv2));
		avm_error(buff);

		return;
	}

	avm_clear(lv);
	lv->type = M_NUMBER;
	lv->val_num = autoarithm[inst->op - VOP_ADD](rv1->val_num, rv2->val_num);
}