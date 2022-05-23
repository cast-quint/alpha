/*
 * Dimitrios Koropoulis, 3967
 * CS340 - Spring 2021
 * avm_lib.c
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "avm_table.h"
#include "avm_inst.h"
#include "avm_lib.h"

#define PI 3.14159265
#define RAD(x) ((PI / 180.0)*x)

typedef void (*libfunc_ptr)(void);

static void avmlib_print(void) {

	unsigned int arg_count;
	size_t i;

	arg_count = avm_total_args();
	for (i = 0; i < arg_count; ++i) {
		printf("%s ", avm_tostring(avm_get_arg(i)));
	}
	puts("");

	avm_clear(erx);
	erx->type = M_NIL;
}

static void avmlib_input(void) {

	char input[1024];
 	char* end = NULL;
	double num_input;
	unsigned int arg_count = avm_total_args();
	Memcell jack;


	if (arg_count != 0) {
		snprintf(input, 1024, "\"input\" expects zero arguments (not %u)", arg_count);
		avm_error(input);
		return;
	}

	fgets(input, 1024, stdin);

	if (input[0] == '"' && input[strlen(input) - 2] == '"') {
		jack.type = M_STRING;
		input[strlen(input) - 2] = '\0';
		end = input + 1;
		jack.val_string = end;
	} else if (!strcmp(input, "true\n")) {
		jack.type = M_BOOL;
		jack.val_bool = 1;
	} else if (!strcmp(input, "false\n")) {
		jack.type = M_BOOL;
		jack.val_bool = 0;
	} else if (!strcmp(input, "nil\n")) {
		jack.type = M_NIL;
	} else {
		num_input = strtod(input, &end);
		if (*end == '\n' && input[0] != '\n') {
			jack.type = M_NUMBER;
			jack.val_num = num_input;
		} else {
			jack.type = M_STRING;
			input[strlen(input) - 1] = '\0';
			jack.val_string = input;
		}
	}

	avm_clear(erx);
	avm_assign(erx, &jack);
}

static void avmlib_totalarguments(void) {

	unsigned int topsp_prev = avm_get_env_val(topsp + AVM_SAVED_TOPSP_OFFSET);
	unsigned int arg_count = avm_total_args();
	char buff[1024];

	if (arg_count != 0) {
		snprintf(buff, 1024, "\"totalarguments\" expects zero arguments (not %u)", arg_count);
		avm_error(buff);
		return;
	}

	avm_clear(erx);
	if (topsp_prev == AVM_STACK_SIZE - 1) {
		erx->type = M_NIL;
		return;
	}

	erx->type = M_NUMBER;
	erx->val_num = avm_get_env_val(topsp_prev + AVM_SAVED_ARG_COUNT_OFFSET);
}

static void avmlib_argument(void) {

	unsigned int topsp_prev = avm_get_env_val(topsp + AVM_SAVED_TOPSP_OFFSET);
	unsigned int arg_count = avm_total_args();
	unsigned int callee_arg_count = avm_get_env_val(topsp_prev + AVM_SAVED_ARG_COUNT_OFFSET);
	Memcell* offset_memcell = avm_get_arg(0);
	char buff[1024];

	if (arg_count != 1) {
		snprintf(buff, 1024, "\"argument\" expects one argument (not %u)", arg_count);
		avm_error(buff);
		return;
	}

	if (offset_memcell->type != M_NUMBER) {
		snprintf(buff, 1024, "\"argument\" expects a number (not %s)", avm_string_types[offset_memcell->type]);
		avm_error(buff);
		return;
	}

	if (!avm_is_int(offset_memcell->val_num) || offset_memcell->val_num < 0) {
		avm_error("\"argument\" expects a positive integer (the number of an actual argument of the callee, starting from 0)");
		return;
	}

	if (offset_memcell->val_num >= callee_arg_count) {
		snprintf(buff, 1024, "callee has only %u arguments", callee_arg_count);
		avm_error(buff);
		return;
	}

	avm_clear(erx);
	if (topsp_prev == AVM_STACK_SIZE - 1) {
		erx->type = M_NIL;
		return;
	}

	avm_assign(erx, &avm_stack[topsp_prev + AVM_CALLENV_SIZE + 1 + (unsigned int)offset_memcell->val_num]);
}

static void avmlib_typeof(void) {

	unsigned int arg_count = avm_total_args();
	char buff[1024];

	if (arg_count != 1) {
		snprintf(buff, 1024, "\"typeof\" expects one argument (not %u)", arg_count);
		avm_error(buff);
		return;
	}

	avm_clear(erx);
	erx->type = M_STRING;
	erx->val_string = strdup(avm_string_types[avm_get_arg(0)->type]);
}

static void avmlib_strtonum(void) {

	char buff[1024];
	char* end = NULL;
	double num;
	unsigned int arg_count = avm_total_args();
	Memcell* arg = avm_get_arg(0);
	Memcell jack;


	if (arg_count != 1) {
		snprintf(buff, 1024, "\"strtonum\" expects one argument (not %u)", arg_count);
		avm_error(buff);
		return;
	}

	if (arg->type != M_STRING) {
		snprintf(buff, 1024, "\"strtonum\" expects a string (not %s)", avm_string_types[arg->type]);
		avm_error(buff);
		return;
	}

	num = strtod(arg->val_string, &end);
	printf("%c\n", *end);
	if (*end == '\0') {
		jack.type = M_NUMBER;
		jack.val_num = num;
	} else {
		jack.type = M_NIL;
	}

	avm_clear(erx);
	avm_assign(erx, &jack);
}

static void avmlib_sqrt(void) {

	char buff[1024];
	unsigned int arg_count = avm_total_args();
	Memcell* arg = avm_get_arg(0);
	Memcell jack;

	if (arg_count != 1) {
		snprintf(buff, 1024, "\"sqrt\" expects one argument (not %u)", arg_count);
		avm_error(buff);
		return;
	}

	if (arg->type != M_NUMBER) {
		snprintf(buff, 1024, "\"sqrt\" expects a number (not %s)", avm_string_types[arg->type]);
		avm_error(buff);
		return;
	}

	if (arg->val_num < 0) {
		jack.type = M_NIL;
	} else {
		jack.type = M_NUMBER;
		jack.val_num = sqrt(arg->val_num);
	}

	avm_clear(erx);
	avm_assign(erx, &jack);
}

static void avmlib_cos(void) {

	char buff[1024];
	unsigned int arg_count = avm_total_args();
	Memcell* arg = avm_get_arg(0);
	Memcell jack;

	if (arg_count != 1) {
		snprintf(buff, 1024, "\"cos\" expects one argument (not %u)", arg_count);
		avm_error(buff);
		return;
	}

	if (arg->type != M_NUMBER) {
		snprintf(buff, 1024, "\"cos\" expects a number (not %s)", avm_string_types[arg->type]);
		avm_error(buff);
		return;
	}

	jack.type = M_NUMBER;
	jack.val_num = cos(RAD(arg->val_num));

	avm_clear(erx);
	avm_assign(erx, &jack);
}

static void avmlib_sin(void) {

	char buff[1024];
	unsigned int arg_count = avm_total_args();
	Memcell* arg = avm_get_arg(0);
	Memcell jack;

	if (arg_count != 1) {
		snprintf(buff, 1024, "\"sin\" expects one argument (not %u)", arg_count);
		avm_error(buff);
		return;
	}

	if (arg->type != M_NUMBER) {
		snprintf(buff, 1024, "\"sin\" expects a number (not %s)", avm_string_types[arg->type]);
		avm_error(buff);
		return;
	}

	jack.type = M_NUMBER;
	jack.val_num = sin(RAD(arg->val_num));

	avm_clear(erx);
	avm_assign(erx, &jack);
}

static void avmlib_objectmemberkeys(void) {

	memcell_t t;
	unsigned int i;
	unsigned int arg_count = 0;
	unsigned int indexed_sum = 0;
	unsigned int key_count = 0;
	Memcell* arg = NULL;
	TableBucket* current = NULL;
	Memcell jack;
	Table* new_table = NULL;
	char buff[1024];

	arg_count = avm_total_args();
	arg = avm_get_arg(0);

	new_table = avm_new_table();
	avm_increment_ref(new_table);

	if (arg_count != 1) {
		snprintf(buff, 1024, "\"objectmemberkeys\" expects one argument (not %u)", arg_count);
		avm_error(buff);
		return;
	}

	if (arg->type != M_TABLE) {
		snprintf(buff, 1024, "\"objectmemberkeys\" expects a table (not %s)", avm_string_types[arg->type]);
		avm_error(buff);
		return;
	}

	for (t = 0; t < AVM_VALID_TYPES_COUNT; ++t) {
		indexed_sum += arg->val_table->indexed_total[t];
	}

	if (indexed_sum == 0) {
		erx->type = M_TABLE;
		erx->val_table = new_table;
		return;
	}

	for (t = 0; t < AVM_VALID_TYPES_COUNT; ++t) {

		if (arg->val_table->indexed_total[t] == 0) {
			continue;
		}

		for (i = 0; i < AVM_HASHTABLE_SIZE; ++i) {
			current = arg->val_table->indexed[t][i];
			while (current) {

				if (current->value->type != M_NIL && current->value->type != M_UNDEF) {
					jack.type = M_NUMBER;
					jack.val_num = key_count++;
					avm_tablesetelem(new_table, &jack, current->key);
				}

				current = current->next;
			}
		}
	}

	assert(indexed_sum == key_count);

	avm_clear(erx);
	erx->type = M_TABLE;
	erx->val_table = new_table;
}

static void avmlib_objecttotalmembers(void) {

	memcell_t t;
	char buff[1024];
	Memcell* arg = NULL;
	unsigned int sum = 0;
	unsigned int arg_count;

	arg_count = avm_total_args();
	arg = avm_get_arg(0);

	if (arg_count != 1) {
		snprintf(buff, 1024, "\"objecttotalmembers\" expects one argument (not %u)", arg_count);
		avm_error(buff);
		return;
	}

	if (arg->type != M_TABLE) {
		snprintf(buff, 1024, "\"objecttotalmembers\" expects a table (not %s)", avm_string_types[arg->type]);
		avm_error(buff);
		return;
	}

	for (t = 0; t < AVM_VALID_TYPES_COUNT; ++t) {
		sum += arg->val_table->indexed_total[t];
	}


	avm_clear(erx);
	erx->type = M_NUMBER;
	erx->val_num = sum;
}

static void avmlib_objectcopy(void) {

	memcell_t t;
	unsigned int i;
	char buff[1024];
	Memcell* arg = NULL;
	unsigned int arg_count;
	Table* new_table = NULL;
	TableBucket* current = NULL;

	arg_count = avm_total_args();
	arg = avm_get_arg(0);

	new_table = avm_new_table();
	avm_increment_ref(new_table);

	if (arg_count != 1) {
		snprintf(buff, 1024, "\"objectcopy\" expects one argument (not %u)", arg_count);
		avm_error(buff);
		return;
	}

	if (arg->type != M_TABLE) {
		snprintf(buff, 1024, "\"objectcopy\" expects a table (not %s)", avm_string_types[arg->type]);
		avm_error(buff);
		return;
	}

	for (t = 0; t < AVM_VALID_TYPES_COUNT; ++t) {

		if (arg->val_table->indexed_total[t] == 0) {
			continue;
		}

		for (i = 0; i < AVM_HASHTABLE_SIZE; ++i) {
			current = arg->val_table->indexed[t][i];
			while (current) {
				if (current->value->type != M_NIL ) {
					avm_tablesetelem(new_table, current->key, current->value);
				}

				current = current->next;
			}
		}
	}

	avm_clear(erx);
	erx->type = M_TABLE;
	erx->val_table = new_table;
}

static libfunc_ptr avm_get_lib(const char* name) {

	if (strcmp(name, "print") == 0) {
		return avmlib_print;
	}

	if (strcmp(name, "input") == 0) {
		return avmlib_input;
	}

	if (strcmp(name, "objectmemberkeys") == 0) {
		return avmlib_objectmemberkeys;
	}

	if (strcmp(name, "objecttotalmembers") == 0) {
		return avmlib_objecttotalmembers;
	}

	if (strcmp(name, "objectcopy") == 0) {
		return avmlib_objectcopy;
	}

	if (strcmp(name, "totalarguments") == 0) {
		return avmlib_totalarguments;
	}

	if (strcmp(name, "argument") == 0) {
		return avmlib_argument;
	}

	if (strcmp(name, "typeof") == 0) {
		return avmlib_typeof;
	}

	if (strcmp(name, "strtonum") == 0) {
		return avmlib_strtonum;
	}

	if (strcmp(name, "sqrt") == 0) {
		return avmlib_sqrt;
	}

	if (strcmp(name, "cos") == 0) {
		return avmlib_cos;
	}

	if (strcmp(name, "sin") == 0) {
		return avmlib_sin;
	}

	return NULL;
}

unsigned int avm_total_args(void) {

	//puts("getting total arguments...");
	return avm_get_env_val(topsp + AVM_SAVED_ARG_COUNT_OFFSET);
}

Memcell* avm_get_arg(unsigned int i) {

	assert(i < avm_total_args());

	return &avm_stack[topsp + AVM_CALLENV_SIZE + 1 + i];
}

void avm_call_lib(const char* name) {

	char buff[1024];

	libfunc_ptr libfunc = avm_get_lib(name);

	if (!libfunc) {
		snprintf(buff, 1024, "unsupported library function \"%s\" called", name);
		avm_error(buff);
		return;
	}

	/* manual enterfunc & exitfunc sequence */

	topsp = top;
	total_actual_args = 0;
	libfunc();

	if (!execution_done) {
		execute_exitfunc(NULL);
	}
}







