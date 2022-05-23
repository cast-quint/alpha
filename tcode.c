/*
 * Dimitrios Koropoulis, 3967
 * CS340 - Spring 2021
 * tcode.c
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "memory-management.h"
#include "tcode.h"
#include "util.h"


#define EXPAND_SIZE  1024

#define CURR_NUMTABLE_SIZE	(g_numtable_count * sizeof(double))
#define NEW_NUMTABLE_SIZE	(CURR_NUMTABLE_SIZE + EXPAND_SIZE * sizeof(double))

#define CURR_STRTABLE_SIZE	(g_strtable_count * sizeof(char *))
#define NEW_STRTABLE_SIZE  	(CURR_STRTABLE_SIZE + EXPAND_SIZE * sizeof(char *))

#define CURR_LIBTABLE_SIZE	(g_libtable_count * sizeof(char *))
#define NEW_LIBTABLE_SIZE	(CURR_LIBTABLE_SIZE + EXPAND_SIZE * sizeof(char *))

#define CURR_FUNCTABLE_SIZE (g_functable_count * sizeof(Function *))
#define NEW_FUNCTABLE_SIZE  (CURR_FUNCTABLE_SIZE + EXPAND_SIZE * sizeof(Function *))

#define CURR_INST_SIZE      (g_inst_size * sizeof(Instruction *))
#define NEW_INST_SIZE       (CURR_INST_SIZE + EXPAND_SIZE * sizeof(Instruction *))

extern unsigned int g_offset_program;

static IncompleteJump* g_jump_list = NULL;
static unsigned int g_jump_count = 0;

static double* g_numtable = NULL;
static unsigned int g_numtable_size = 0;
static unsigned int g_numtable_count = 0;

static const char** g_strtable = NULL;
static unsigned int g_strtable_size = 0;
static unsigned int g_strtable_count = 0;

static const char** g_libtable = NULL;
static unsigned int g_libtable_size = 0;
static unsigned int g_libtable_count = 0;

static Function** g_functable = NULL;
static unsigned int g_functable_size = 0;
static unsigned int g_functable_count = 0;

static Instruction** g_instructions = NULL;
static unsigned int g_inst_size = 0;
static unsigned int g_inst_count = 0;

static Arg* g_retval;
static FILE* g_tcodeout;

static ReturnStack* g_retstack = NULL;

static void insert_return(IncompleteReturn** head, unsigned int inst_num) {

    IncompleteReturn* ret = NULL;

    ret = mymalloc(sizeof(IncompleteReturn));
    ret->inst_num = inst_num;
    ret->next = NULL;

    if (!*head) {
        *head = ret;
        return;
    }

    ret->next = *head;
    *head = ret;
}

static void make_stack(void) {

    assert(!g_retstack);

    g_retstack = mymalloc(sizeof(ReturnStack));
    g_retstack->top = 0;
}

static void push(IncompleteReturn* return_list) {


    if (!g_retstack) {
        make_stack();
    }

    assert(g_retstack);
    assert(g_retstack->top <= MAX_STACK_SIZE);

    g_retstack->items[(g_retstack->top)++] = return_list;
}

static IncompleteReturn* pop(void) {

    assert(g_retstack);
    assert(g_retstack->top);

    return g_retstack->items[--(g_retstack->top)];
}

static IncompleteReturn** peek(void) {

    assert(g_retstack);
    assert(g_retstack->top);

    return &(g_retstack->items[g_retstack->top - 1]);
}

static void insert_jump(unsigned int quad_num, unsigned int jump_label) {

    IncompleteJump* jump = NULL;

    assert(quad_num);
    assert(jump_label);

    jump = mymalloc(sizeof(IncompleteJump));
    jump->quad_num = quad_num;
    jump->jump_label = jump_label;
    jump->next = NULL;


    if (!g_jump_list) {
        g_jump_list = jump;
        return;
    }

    jump->next = g_jump_list;
    g_jump_list = jump;

    ++g_jump_count;
}

static size_t current_inst(void) {

    return g_inst_count; // TODO CHECK;
}

static void inst_expand(void) {

    Instruction** new_inst = NULL;

    assert(g_inst_count == g_inst_size);

    new_inst = malloc(NEW_INST_SIZE);
    if (g_instructions) {
        memcpy(new_inst, g_instructions, CURR_INST_SIZE);
        free(g_instructions);
    }

    g_instructions = new_inst;
    g_inst_size += EXPAND_SIZE;
}

static void numtable_expand(void) {

    double* new_table = NULL;

    assert(g_numtable_count == g_numtable_size);

    new_table = calloc(g_numtable_size + EXPAND_SIZE, sizeof(double));
    if (g_numtable) {
        memcpy(new_table, g_numtable, CURR_NUMTABLE_SIZE);
        free(g_numtable);
    } else {

    }

    g_numtable = new_table;
    g_numtable_size += EXPAND_SIZE;
}

static void strtable_expand(void) {
    const char** new_table = NULL;

    assert(g_strtable_count == g_strtable_size);

    new_table = malloc(NEW_STRTABLE_SIZE);
    if (g_strtable) {
        memcpy(new_table, g_strtable, CURR_STRTABLE_SIZE);
        free(g_strtable);
    }

    g_strtable = new_table;
    g_strtable_size += EXPAND_SIZE;
}

static void libtable_expand(void) {
    const char** new_table = NULL;

    assert(g_libtable_count == g_libtable_size);

    new_table = malloc(NEW_LIBTABLE_SIZE);
    if (g_libtable) {
        memcpy(new_table, g_libtable, CURR_LIBTABLE_SIZE);
        free(g_libtable);
    }

    g_libtable = new_table;
    g_libtable_size += EXPAND_SIZE;
}

static void functable_expand(void) {
    Function** new_table = NULL;

    assert(g_functable_count == g_functable_size);

    new_table = malloc(NEW_FUNCTABLE_SIZE);
    if (g_functable) {
        memcpy(new_table, g_functable, CURR_FUNCTABLE_SIZE);
        free(g_functable);
    }

    g_functable = new_table;
    g_functable_size += EXPAND_SIZE;
}

static size_t get_const_num(double num) {

    size_t i;

    for (i = 0; i < g_numtable_count; i++) {
        if (g_numtable[i] == num) {
            return i;
        }
    }

    if (g_numtable_count == g_numtable_size) {
        numtable_expand();
    }

    g_numtable[g_numtable_count] = num;

    return g_numtable_count++;
}

static size_t get_const_str(const char* str) {

    size_t i;

    for (i = 0; i < g_strtable_count; i++) {
        if (strcmp(g_strtable[i], str) == 0) {
            return i;
        }
    }

    if (g_strtable_count == g_strtable_size) {
        strtable_expand();
    }

    g_strtable[g_strtable_count] = str;

    return g_strtable_count++;
}

static size_t get_const_lib(const char* name) {

    size_t i;

    for (i = 0; i < g_libtable_count; i++) {
        if (strcmp(g_libtable[i], name) == 0) {
            return i;
        }
    }

    if (g_libtable_count == g_libtable_size) {
        libtable_expand();
    }

    g_libtable[g_libtable_count] = name;

    return g_libtable_count++;
}

static size_t get_user_func(Expression* e) {

    size_t i;

    assert(e->type == E_PROGRAMFUNC);

    for (i = 0; i < g_functable_count; i++) {
        if (strcmp(g_functable[i]->name, e->symbol->name) == 0) {
            return i;
        }
    }

    if (g_functable_count == g_functable_size) {
        functable_expand();
    }

    g_functable[g_functable_count] = mymalloc(sizeof(Function));
    g_functable[g_functable_count]->name = e->symbol->name;
    g_functable[g_functable_count]->taddress = current_inst();
    g_functable[g_functable_count]->local_size = e->symbol->local_count;

    return g_functable_count++;
}


static Arg* make_label(unsigned int label) {

    Arg* arg = NULL;


    arg = mymalloc(sizeof(Arg));
    arg->type = A_LABEL;
    arg->val = label;

    return arg;
}


static void generate_ASSIGN(Quad* quad) {

	Arg* result = NULL;
	Arg* arg1 = NULL;

	assert(quad);

    quad->taddress = current_inst();

    result = make_arg(quad->result);
	arg1 = make_arg(quad->arg1);

	emit_inst(VOP_ASSIGN, result, arg1, NULL, quad->line);
}

static void generate_ADD(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();

    result = make_arg(quad->result);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(quad->arg2);

    emit_inst(VOP_ADD, result, arg1, arg2, quad->line);
}

static void generate_SUB(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();

    result = make_arg(quad->result);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(quad->arg2);

    emit_inst(VOP_SUB, result, arg1, arg2, quad->line);
}

static void generate_MUL(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();

    result = make_arg(quad->result);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(quad->arg2);

    emit_inst(VOP_MUL, result, arg1, arg2, quad->line);
}

static void generate_DIV(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();

    result = make_arg(quad->result);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(quad->arg2);

    emit_inst(VOP_DIV, result, arg1, arg2, quad->line);
}

static void generate_MOD(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();

    result = make_arg(quad->result);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(quad->arg2);

    emit_inst(VOP_MOD, result, arg1, arg2, quad->line);
}

static void generate_UMINUS(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();

    result = make_arg(quad->result);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(g_minus);

    emit_inst(VOP_MUL, result, arg1, arg2, quad->line);
}

static void generate_IF_EQ(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();
    insert_jump(quad->iaddress, quad->label);

    result = make_label(quad->label);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(quad->arg2);

    emit_inst(VOP_JEQ, result, arg1, arg2, quad->line);
}

static void generate_IF_NOTEQ(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();
    insert_jump(quad->iaddress, quad->label);

    result = make_label(quad->label);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(quad->arg2);

    emit_inst(VOP_JNE, result, arg1, arg2, quad->line);
}

static void generate_IF_GREATER(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();
    insert_jump(quad->iaddress, quad->label);

    result = make_label(quad->label);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(quad->arg2);

    emit_inst(VOP_JGT, result, arg1, arg2, quad->line);
}

static void generate_IF_LESS(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();
    insert_jump(quad->iaddress, quad->label);

    result = make_label(quad->label);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(quad->arg2);

    emit_inst(VOP_JLT, result, arg1, arg2, quad->line);
}

static void generate_IF_GREATEREQ(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();
    insert_jump(quad->iaddress, quad->label);

    result = make_label(quad->label);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(quad->arg2);

    emit_inst(VOP_JGE, result, arg1, arg2, quad->line);
}

static void generate_IF_LESSEQ(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();
    insert_jump(quad->iaddress, quad->label);

    result = make_label(quad->label);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(quad->arg2);

    emit_inst(VOP_JLE, result, arg1, arg2, quad->line);
}

static void generate_JUMP(Quad* quad) {

    Arg* result = NULL;

    assert(quad);

    quad->taddress = current_inst();
    insert_jump(quad->iaddress, quad->label);

    result = make_label(quad->label);
    emit_inst(VOP_JUMP, result, NULL, NULL, quad->line);
}

static void generate_TABLECREATE(Quad* quad) {

    Arg* result = NULL;

    assert(quad);

    quad->taddress = current_inst();

    result = make_arg(quad->result);

    emit_inst(VOP_NEWTABLE, result, NULL, NULL, quad->line);
}

static void generate_TABLEGETELEM(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();

    result = make_arg(quad->result);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(quad->arg2);

    emit_inst(VOP_TABLEGETELEM, result, arg1, arg2, quad->line);
}

static void generate_TABLESETELEM(Quad* quad) {

    Arg* result = NULL;
    Arg* arg1 = NULL;
    Arg* arg2 = NULL;

    assert(quad);

    quad->taddress = current_inst();

    result = make_arg(quad->result);
    arg1 = make_arg(quad->arg1);
    arg2 = make_arg(quad->arg2);

    emit_inst(VOP_TABLESETELEM, result, arg1, arg2, quad->line);
}

static void generate_CALL(Quad* quad) {

    Arg* result = NULL;

    assert(quad);

    quad->taddress = current_inst();

    result = make_arg(quad->result);

    emit_inst(VOP_CALLFUNC, result, NULL, NULL, quad->line);
}

static void generate_FUCNSTART(Quad* quad) {

    Arg* result = NULL;

    assert(quad);

    push(NULL);

    quad->taddress = current_inst();
    result = make_arg(quad->result);

    emit_inst(VOP_ENTERFUNC, result, NULL, NULL, quad->line);
}

static void generate_FUNCEND(Quad* quad) {

    Arg* result = NULL;
    IncompleteReturn* ret_list;

    assert(quad);

    ret_list = pop();
    while (ret_list) {
        g_instructions[ret_list->inst_num]->result->val = current_inst();
        ret_list = ret_list->next;
    }


    quad->taddress = current_inst();
    result = make_arg(quad->result);

    emit_inst(VOP_EXITFUNC, result, NULL, NULL, quad->line);
}

static void generate_PARAM(Quad* quad) {

    Arg* result = NULL;

    assert(quad);

    quad->taddress = current_inst();

    result = make_arg(quad->result);

    emit_inst(VOP_PUSHARG, result, NULL, NULL, quad->line);
}

static void generate_RETURN(Quad* quad) {

    Arg* arg1 = NULL;
    Arg* label = NULL;

    assert(quad);

    insert_return(peek(), current_inst() + (quad->result ? 1 : 0));

    quad->taddress = current_inst();
    label = make_label(0);

    if (quad->result) {
        arg1 = make_arg(quad->result);
        emit_inst(VOP_ASSIGN, g_retval, arg1, NULL, quad->line);
    }

    emit_inst(VOP_JUMP, label, NULL, NULL, quad->line);
}

static void generate_GETRETVAL(Quad* quad) {

    Arg* result = NULL;

    assert(quad);

    quad->taddress = current_inst();

    result = make_arg(quad->result);

    emit_inst(VOP_ASSIGN, result, g_retval, NULL, quad->line);
}

static void (*autogen[])(Quad*) = {
    generate_ASSIGN,

    generate_ADD,
    generate_SUB,
    generate_MUL,
    generate_DIV,
    generate_MOD,
    generate_UMINUS,

    generate_IF_EQ,
    generate_IF_NOTEQ,
    generate_IF_GREATER,
    generate_IF_LESS,
    generate_IF_GREATEREQ,
    generate_IF_LESSEQ,
    generate_JUMP,

    generate_TABLECREATE,
    generate_TABLEGETELEM,
    generate_TABLESETELEM,

    generate_CALL,
    generate_FUCNSTART,
    generate_FUNCEND,
    generate_PARAM,
    generate_RETURN,
    generate_GETRETVAL
};

char* print_vopcode(vopcode op) {

    switch(op) {
        case VOP_ASSIGN: {
            return "ASSIGN";
        }

        case VOP_ADD: {
            return "ADD";
        }

        case VOP_SUB: {
            return "SUB";
        }

        case VOP_MUL: {
            return "MUL";
        }

        case VOP_DIV: {
            return "DIV";
        }

        case VOP_MOD: {
            return "MOD";
        }

        case VOP_JEQ: {
            return "JEQ";
        }

        case VOP_JNE: {
            return "JNE";
        }

        case VOP_JGT: {
            return "JGT";
        }
        case VOP_JLT: {
            return "JLT";
        }

        case VOP_JGE: {
            return "JGE";
        }

        case VOP_JLE: {
            return "JLE";
        }

        case VOP_JUMP: {
            return "JUMP";
        }

        case VOP_NEWTABLE: {
            return "NEWTABLE";
        }

        case VOP_TABLEGETELEM: {
            return "TABLEGETELEM";
        }

        case VOP_TABLESETELEM: {
            return "TABLESETELEM";
        }

        case VOP_ENTERFUNC: {
            return "ENTERFUNC";
        }

        case VOP_EXITFUNC: {
            return "EXITFUNC";
        }

        case VOP_CALLFUNC: {
            return "CALLFUNC";
        }

        case VOP_PUSHARG: {
            return "PUSHARG";
        }

        case VOP_NOP: {
            return "NOP";
        }

        default: {
            assert(0);
        }
    }
}

static char* print_type(arg_type type) {

    switch(type) {

        case A_LABEL: {
            return "label";
        }

        case A_GLOBAL: {
            return "global";
        }

        case A_FORMAL: {
            return "formal";
        }

        case A_LOCAL: {
            return "local";
        }

        case A_NUM: {
            return "num";
        }

        case A_LIBFUNC: {
            return "libfunc";
        }

        case A_STRING: {
            return "string";
        }

        case A_BOOL: {
            return "bool";
        }

        case A_USERFUNC: {
            return "userfunc";
        }

        case A_RETVAL: {
            return "retval";
        }

        case A_NIL: {
            return "nil";
        }

        default: {
            assert(0);
        }
    }
}

static char* print_val(arg_type type, unsigned int val) {

    switch (type) {

        case A_LABEL: {
            snprintf(g_genbuff, GENBUFF_LEN, "%u", val);
            return mystrdup(g_genbuff);
        }

        case A_NUM: {
            snprintf(g_genbuff, GENBUFF_LEN, "(%.2lf)", g_numtable[val]);
            return mystrdup(g_genbuff);
        }

        case A_STRING: {
            snprintf(g_genbuff, GENBUFF_LEN, "(\"%s\")", g_strtable[val]);
            return mystrdup(g_genbuff);
        }

        /*case A_LIBFUNC: {
            snprintf(g_genbuff, GENBUFF_LEN, "%2u(%s)", val, g_libtable[val]);
            return mystrdup(g_genbuff);
        }

        case A_USERFUNC: {
            snprintf(g_genbuff, GENBUFF_LEN, "%2u(%s)", val, g_functable[val]->name);
            return mystrdup(g_genbuff);
        }*/

        default: {
            snprintf(g_genbuff, GENBUFF_LEN, "%02u", val);
            return mystrdup(g_genbuff);
        }
    }
}

char* print_arg(Arg* arg) {

    if (!arg || arg->type == A_DUMMY) {
        return "";
    }

    if (arg->type == A_RETVAL) {
        snprintf(g_genbuff, GENBUFF_LEN, "%8s", print_type(arg->type));
        return mystrdup(g_genbuff);
    }

    snprintf(g_genbuff, GENBUFF_LEN, "%8s:%s", print_type(arg->type), print_val(arg->type, arg->val));
    return mystrdup(g_genbuff);
}

char* make_bytefilename(void) {

    const char path_delim[2] = "/";
    const char file_delim[2] = ".";
    char* prev_token = NULL;
    char* curr_token = NULL;
    char path[512];

    strncpy(path, g_input_filename, 512);

    curr_token = strtok(path, path_delim);
    while (curr_token != NULL) {
        prev_token = curr_token;
        curr_token = strtok(NULL, path_delim);
    }

    prev_token = strtok(prev_token, file_delim);

    snprintf(g_genbuff, GENBUFF_LEN, "%s.abc", prev_token);

    return mystrdup(g_genbuff);
}

void tcode_init(void) {

    if (g_log) {
	   g_tcodeout = myfopen("logs/target.txt", "w");
    }

    g_retval = mymalloc(sizeof(Arg));
    g_retval->type = A_RETVAL;
    g_retval->val = 0;
}

void tcode_clean(void) {

	free(g_numtable);
	free(g_strtable);
	free(g_functable);
	free(g_libtable);

	free(g_instructions);

    if (g_log) {
        fclose(g_tcodeout);
    }
}

void emit_inst(vopcode op, Arg* result, Arg* arg1, Arg* arg2, unsigned int line) {

	Instruction* new_inst = NULL;

	assert(result);

	new_inst = mymalloc(sizeof(Instruction));

    /* suppress some valgrind warnings */
    memset(new_inst, 0, sizeof(Instruction));

	new_inst->op = op;
	new_inst->result = result;
	new_inst->arg1 = arg1;
	new_inst->arg2 = arg2;
    new_inst->line = line;

	if (g_inst_count == g_inst_size) {
		inst_expand();
	}

	g_instructions[g_inst_count++] = new_inst;
}

Arg* make_arg(Expression* e) {

	Arg* arg = NULL;

	arg = mymalloc(sizeof(Arg));

	switch (e->type) {

		/* VARIABLES */
		case E_VAR:
        case E_ASSIGNEXPR:
        case E_TABLEITEM:
        case E_NEWTABLE:
        case E_ARITHMEXPR:
        case E_BOOLEXPR: {

        	assert(e->symbol);

        	arg->val = e->symbol->offset;
        	switch(e->symbol->space) {
        		case SC_PROGRAM: {
        			arg->type = A_GLOBAL;
        			break;
        		}

        		case SC_LOCAL: {
        			arg->type = A_LOCAL;
        			break;
        		}

        		case SC_FORMAL: {
        			arg->type = A_FORMAL;
        			break;
        		}

        		default: {
        			assert(0);
        		}
        	}
        	break;
        }

        /* CONSTANTS */

        case E_CONSTBOOL: {
        	arg->type = A_BOOL;
        	arg->val = e->const_bool;
        	break;
        }

        case E_CONSTNUM: {
        	arg->type = A_NUM;
        	arg->val = get_const_num(e->const_num);
        	break;
        }

        case E_CONSTSTRING: {
        	arg->type = A_STRING;
        	arg->val = get_const_str(e->const_string);
        	break;
        }

        /* FUNCTIONS */

        case E_PROGRAMFUNC: {
        	arg->type = A_USERFUNC;
        	arg->val = get_user_func(e);
        	break;
        }

        case E_LIBRARYFUNC: {
        	arg->type = A_LIBFUNC;
        	arg->val = get_const_lib(e->symbol->name);
        	break;
        }

        case E_NIL: {
            arg->type = A_NIL;
            arg->val = 0;
            break;
        }

        default: {
        	assert(0);
        }
	}

	return arg;
}

void patch_jumps(void) {

    IncompleteJump* current = NULL;
    Instruction* tcode_jump = NULL;
    Quad*        icode_jump = NULL;

    current = g_jump_list;
    while (current) {

        icode_jump = g_quad_array[current->quad_num - 1];
        tcode_jump = g_instructions[icode_jump->taddress];


        /* if the icode jump label points the end of the icode,
         * just put the end of the tcode as the val
         */
        if (icode_jump->label == next_quad()) {
            tcode_jump->result->val =  g_inst_count;
            current = current->next;
            continue;
        }

        tcode_jump->result->val = g_quad_array[icode_jump->label - 1]->taddress;

        current = current->next;
    }
}

void generate_tcode(void) {

    size_t i;

	for (i = 0; i < current_quad(); i++) {
        /* we assign the count/number [1, ...] of a quad
         * NOT its index in the quad_array [0, ...]
         */
        g_quad_array[i]->iaddress = i + 1;
        autogen[g_quad_array[i]->op](g_quad_array[i]);
	}

    patch_jumps();
}

void write_bytecode(void) {

    unsigned int magic = MAGIC_NUMBER;
    unsigned long int size;
    FILE* bytefile = NULL;
    size_t i;
    Arg dummy;
    dummy.type = A_DUMMY;
    dummy.val = 0;

    bytefile = myfopen(make_bytefilename(), "w");


    /* magic number */
    fwrite(&magic, sizeof(unsigned int), 1, bytefile);

    /* global offset */
    fwrite(&g_offset_program, sizeof(unsigned int), 1, bytefile);

    /* Strings */
    fwrite(&g_strtable_count, sizeof(unsigned int), 1, bytefile);
    for (i = 0; i < g_strtable_count; i++) {
        size = strlen(g_strtable[i]);
        fwrite(&size, sizeof(unsigned long int), 1, bytefile);
        fwrite(g_strtable[i], sizeof(char), size + 1, bytefile);
    }

    /* Numbers */
    fwrite(&g_numtable_count, sizeof(unsigned int), 1, bytefile);
    for (i = 0; i < g_numtable_count; i++) {
        fwrite(&g_numtable[i], sizeof(double), 1, bytefile);
    }

    /* User functions */
    fwrite(&g_functable_count, sizeof(unsigned int), 1, bytefile);
    for (i = 0; i < g_functable_count; i++) {
        fwrite(&g_functable[i]->taddress, sizeof(unsigned int), 1, bytefile);
        fwrite(&g_functable[i]->local_size, sizeof(unsigned int), 1, bytefile);

        size = strlen(g_functable[i]->name);
        fwrite(&size, sizeof(unsigned long int), 1, bytefile);
        fwrite(g_functable[i]->name, sizeof(char), size + 1, bytefile);
    }

    /* Library functions */
    fwrite(&g_libtable_count, sizeof(unsigned int), 1, bytefile);
    for (i = 0; i < g_libtable_count; i++) {
        size = strlen(g_libtable[i]);
        fwrite(&size, sizeof(unsigned long int), 1, bytefile);
        fwrite(g_libtable[i], sizeof(char), size + 1, bytefile);
    }

    /* code */
    fwrite(&g_inst_count, sizeof(unsigned int), 1, bytefile);
    for (i = 0; i < g_inst_count; i++) {
        fwrite(&(g_instructions[i]->op),    sizeof(vopcode),      1, bytefile);
        fwrite(g_instructions[i]->result,   sizeof(Arg),          1, bytefile);

        if (g_instructions[i]->arg1 != NULL) {
            fwrite(g_instructions[i]->arg1, sizeof(Arg), 1, bytefile);
        } else {
            fwrite(&dummy, sizeof(Arg), 1, bytefile);
        }

        if (g_instructions[i]->arg2 != NULL) {
            fwrite(g_instructions[i]->arg2, sizeof(Arg), 1, bytefile);
        } else {
            fwrite(&dummy,sizeof(Arg), 1, bytefile);
        }

        fwrite(&(g_instructions[i]->line), sizeof(unsigned int), 1, bytefile);
    }

    fclose(bytefile);
}

void print_bytecode(void) {

    FILE* bytefile = NULL;
    Function userfunc;
    Instruction inst;
    char buffer[2048];
    double num;
    unsigned int count;
    unsigned long i;
    unsigned long int size;

    inst.result = mymalloc(sizeof(Arg));
    inst.arg1 = mymalloc(sizeof(Arg));
    inst.arg2 = mymalloc(sizeof(Arg));


    bytefile = myfopen(make_bytefilename(), "r");

    fprintf(g_tcodeout, "== %s ==\n\n", "Generated from the bytecode file");

    /* Magic */
    fread(&count, sizeof(unsigned int), 1, bytefile);
    fprintf(g_tcodeout, "magic: %u\n\n", count);

    /* Global count */
    fread(&count, sizeof(unsigned int), 1, bytefile);
    fprintf(g_tcodeout, "Global count: %u\n\n", count);

    /* Strings */
    fread(&count, sizeof(unsigned int), 1, bytefile);
    fprintf(g_tcodeout, "String count: %u\n", count);
    for (i = 0; i < g_strtable_count; i++) {
        fread(&size, sizeof(unsigned long int), 1, bytefile);
        fread(&buffer, sizeof(char), size + 1, bytefile);
        fprintf(g_tcodeout, "%lu: size = %lu, name = \"%s\"\n", i, size, buffer);
    }

    /* Numbers */
    fread(&count, sizeof(unsigned int), 1, bytefile);
    fprintf(g_tcodeout, "\nNumber count: %u\n", count);
    for (i = 0; i < g_numtable_count; i++) {
        fread(&num, sizeof(double), 1, bytefile);
        fprintf(g_tcodeout, "  %lu: %.2lf\n", i, num);
    }

    /* User Functions */
    fread(&count, sizeof(unsigned int), 1, bytefile);
    fprintf(g_tcodeout, "\nFunction count: %u\n", count);
    for (i = 0; i < g_functable_count; i++) {
        fread(&userfunc.taddress, sizeof(unsigned int), 1, bytefile);
        fread(&userfunc.local_size, sizeof(unsigned int), 1, bytefile);

        fread(&size, sizeof(unsigned long int), 1, bytefile);
        userfunc.name = mymalloc((size + 1) * sizeof(char));
        fread((char*)userfunc.name, sizeof(char), size  + 1, bytefile);

        fprintf(g_tcodeout, "%lu: %s, addr: %u, local size: %u\n", i, userfunc.name, userfunc.taddress, userfunc.local_size);
    }

    /* Library functions */
    fread(&count, sizeof(unsigned int), 1, bytefile);
    fprintf(g_tcodeout, "\nLib count: %u\n", count);
    for (i = 0; i < g_libtable_count; i++) {
        fread(&size, sizeof(unsigned long int), 1, bytefile);
        fread(&buffer, sizeof(char), size + 1, bytefile);
        fprintf(g_tcodeout, "%lu: size = %lu, name = %s\n", i, size, buffer);
    }

    /* Code */
    fread(&count, sizeof(unsigned int), 1, bytefile);
    fprintf(g_tcodeout, "\nCode count: %u\n", count);
    for (i = 0; i < g_inst_count; i++) {
        fread(&inst.op,     sizeof(vopcode),      1, bytefile);
        fread(inst.result,  sizeof(Arg),          1, bytefile);
        fread(inst.arg1,    sizeof(Arg),          1, bytefile);
        fread(inst.arg2,    sizeof(Arg),          1, bytefile);
        fread(&inst.line,   sizeof(unsigned int), 1, bytefile);
        fprintf(g_tcodeout, "%4lu: %12s %s %s %s [%u]\n", i, print_vopcode(inst.op), \
            print_arg(inst.result), print_arg(inst.arg1), print_arg(inst.arg2), inst.line);
    }

    fclose(bytefile);
}
