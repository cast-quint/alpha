/*
 * Dimitrios Koropoulis, 3967
 * CS340 - Spring 2021
 * util.c
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include "memory-management.h"
#include "symtable.h"
#include "stack.h"
#include "list.h"
#include "util.h"

#define LIBFUNC_COUNT       12
#define GLOBAL_SCOPE        0

#define EXPAND_SIZE         1024
#define CURR_QUAD_SIZE      (g_icode_size * sizeof(Quad *))
#define NEW_QUAD_SIZE       (CURR_QUAD_SIZE + EXPAND_SIZE * sizeof(Quad *))


#define HEADER_DELIM    "---------------------------------------------"
#define PRINT_DELIM     fprintf(out, "%s\n", HEADER_DELIM)

/******************** PRINTING STUFF *******************************/

#define PRINT_COUNT 7

#define P_ASSN_INDX 0
#define P_ARTH_INDX 1
#define P_RELO_INDX 2
#define P_TBCR_INDX 3
#define P_TBOP_INDX 4
#define P_FUNC_INDX 5
#define P_JUMP_INDX 6

FILE*           g_icodeout;
const char*     g_input_filename;                                 /* the input file name, if specified */

Expression*     g_false;
Expression*     g_true;
Expression*     g_unit;
Expression*     g_minus;
Expression*     g_dummy;

Quad**          g_quad_array;

Stack*          g_loop_stack;
Stack*          g_local_stack;
unsigned int    g_loop_count;

EStack*         g_func_stack;
Expression*     g_current_function;

SymTable*       g_symtable;
SymTableBucket* g_scope_heads[MAX_SCOPE_COUNT];

char            g_genbuff[GENBUFF_LEN];

char**          g_libfuncs;                                 /* string array to the names of all the library functions */
int             g_syntax_error;
int             g_funcdef_canary;                           /* flag indicating if we are inside a function definition */
unsigned int    g_current_quad;
unsigned int    g_current_line;                             /* the current line of the parser */
unsigned int    g_current_scope;                            /* the current scope */
unsigned int    g_formal_count;                             /* tracks the formal parameter count for current function */
unsigned int    g_blockend_line;
unsigned int    g_scopespace_counter;
unsigned int    g_offset_formal;
unsigned int    g_offset_program;
unsigned int    g_offset_local;
unsigned int    g_temp_count;
unsigned int    g_icode_size;

int             g_log;

extern void tcode_init(void);
extern void tcode_clean(void);
extern FILE* g_symout;

/* auxiliary functions used for (mostly) printing */

static int is_const(Expression* expr) {

    expression_type e_type;

    assert(expr);

    e_type = expr->type;

    return (
            e_type == E_CONSTNUM    ||
            e_type == E_CONSTBOOL   ||
            e_type == E_CONSTSTRING ||
            e_type == E_NIL
            );
}

static char* etoa(Expression* e) {

    const char* format = NULL;
    double const_num;

    assert(e);

    if (!is_const(e)) {
        return (char*) e->symbol->name;
    }

    switch (e->type) {
        case E_CONSTNUM: {
            const_num = e->const_num;
            format = (const_num - (int)const_num) ? "%.3f" : "%.0f";
            snprintf(g_genbuff, GENBUFF_LEN, format, const_num);
            return mystrdup(g_genbuff);
        }

        case E_CONSTBOOL: {
            return e->const_bool ? "true" : "false";
        }

        case E_NIL: {
            return "nil";
        }

        case E_CONSTSTRING: {
            snprintf(g_genbuff, GENBUFF_LEN, "\"%s\"", e->const_string);
            return mystrdup(g_genbuff);
        }

        default: {
            assert(0);
        }
    }
}

static const char* ioptoa(iopcode code) {

    switch (code) {
        case OP_ASSIGN: {
            return "ASSIGN";
        }
        case OP_ADD: {
            return "ADD";
        }
        case OP_SUB: {
            return "SUB";
        }
        case OP_MUL: {
            return "MUL";
        }
        case OP_DIV: {
            return "DIV";
        }
        case OP_MOD: {
            return "MOD";
        }
        case OP_UMINUS: {
            return "UMINUS";
        }
        case OP_AND: {
            return "AND";
        }
        case OP_OR: {
            return "OR";
        }
        case OP_NOT: {
            return "NOT";
        }
        case OP_IF_EQ: {
            return "IF_EQ";
        }
        case OP_IF_NOTEQ: {
            return "IF_NOTEQ";
        }
        case OP_IF_LESSEQ: {
            return "IF_LESSEQ";
        }
        case OP_IF_GREATEREQ: {
            return "IF_GREATEREQ";
        }
        case OP_IF_LESS: {
            return "IF_LESS";
        }
        case OP_IF_GREATER: {
            return "IF_GREATER";
        }
        case OP_JUMP: {
            return "JUMP";
        }
        case OP_CALL: {
            return "CALL";
        }
        case OP_PARAM: {
            return "PARAM";
        }
        case OP_RETURN: {
            return "RETURN";
        }
        case OP_GETRETVAL: {
            return "GETRETVAL";
        }
        case OP_FUCNSTART: {
            return "FUNCSTART";
        }
        case OP_FUNCEND: {
            return "FUNCEND";
        }
        case OP_TABLECREATE: {
            return "TABLECREATE";
        }
        case OP_TABLEGETELEM: {
            return "TABLEGETELEM";
        }
        case OP_TABLESETELEM: {
            return "TABLESETELEM";
        }
        default: {
            assert(0);
        }
    }
}

/* print functions */

static void print_assign(FILE* out, Quad* quad) {

    char* result = NULL;
    char* arg1 = NULL;

    assert(quad);

    result = etoa(quad->result);
    arg1 = etoa(quad->arg1);

    fprintf(out, "%s %s %s [%u]\n", "ASSIGN", result, arg1, quad->line);
}

static void print_arithm(FILE* out, Quad* quad) {

    char* result = NULL;
    char* arg1 = NULL;
    char* arg2 = NULL;

    assert(quad);

    result = etoa(quad->result);
    arg1 = etoa(quad->arg1);

    /* UMINUS check */
    arg2 =  quad->arg2 ? etoa(quad->arg2) : "";

    fprintf(out, "%s %s %s %s [%u]\n", ioptoa(quad->op), result, arg1, arg2, quad->line);
}

static void print_relop(FILE* out, Quad* quad) {

    char* arg1 = NULL;
    char* arg2 = NULL;

    assert(quad);

    arg1 = etoa(quad->arg1);
    arg2 = etoa(quad->arg2);

    fprintf(out, "%s %s %s %u [%u]\n", ioptoa(quad->op), arg1, arg2, quad->label, quad->line);
}

static void print_tablecreate(FILE* out, Quad* quad) {

    char* result = NULL;

    assert(quad);

    result = etoa(quad->result);

    fprintf(out, "%s %s [%u]\n", ioptoa(quad->op), result, quad->line);
}

static void print_tableop(FILE* out, Quad* quad) {

    char* result = NULL;
    char* arg1 = NULL;
    char* arg2 = NULL;

    assert(quad);

    result = etoa(quad->result);
    arg1 = etoa(quad->arg1);
    arg2 = etoa(quad->arg2);

    fprintf(out, "%s %s %s %s [%u]\n", ioptoa(quad->op), result, arg1, arg2, quad->line);
}

static void print_func(FILE* out, Quad* quad) {

    char* result;

    assert(quad);

    /* conditional for plain 'return' */
    result = quad->result ? etoa(quad->result) : "";

    fprintf(out, "%s %s [%u]\n", ioptoa(quad->op), result, quad->line);
}

static void print_jump(FILE* out, Quad* quad) {

    assert(quad);
    fprintf(out, "%s %u [%u]\n", "JUMP", quad->label, quad->line);
}

static void (*print_array[PRINT_COUNT])(FILE*, Quad*) = {
    print_assign,
    print_arithm,
    print_relop,
    print_tablecreate,
    print_tableop,
    print_func,
    print_jump
};

static void printquad(FILE* out, Quad* quad) {

    iopcode op;

    assert(quad);

    op = quad->op;
    switch (op) {
        case OP_ASSIGN: {
            print_array[P_ASSN_INDX](out, quad);
            break;
        }

        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
        case OP_UMINUS: {
            print_array[P_ARTH_INDX](out, quad);
            break;
        }

        case OP_IF_EQ:
        case OP_IF_NOTEQ:
        case OP_IF_LESSEQ:
        case OP_IF_GREATEREQ:
        case OP_IF_LESS:
        case OP_IF_GREATER: {
            print_array[P_RELO_INDX](out, quad);
            break;
        }

        case OP_TABLECREATE: {
            print_array[P_TBCR_INDX](out, quad);
            break;
        }

        case OP_TABLEGETELEM:
        case OP_TABLESETELEM: {
            print_array[P_TBOP_INDX](out, quad);
            break;
        }

        case OP_CALL:
        case OP_PARAM:
        case OP_RETURN:
        case OP_GETRETVAL:
        case OP_FUCNSTART:
        case OP_FUNCEND: {
            print_array[P_FUNC_INDX](out, quad);
            break;
        }
        case OP_JUMP: {
            print_array[P_JUMP_INDX](out, quad);
            break;
        }

        default: {
            assert(0);
        }
    }
}

static void print_new_id(SymTableEntry* entry) {

    #ifdef DEBUG

    assert(entry);
    assert(entry->name);

    fprintf(stdout, "%u: new %s: \"%s\"\n", entry->line, \
        symtable_get_symbol_type(entry->type), entry->name);

    #endif
}

static void print_found_id(SymTableEntry* entry) {

    #ifdef DEBUG

    fprintf(stdout, "%u: found %s: \"%s\"\n", g_current_line, \
        symtable_get_symbol_type(entry->type), entry->name);

    #endif
}

/*******************************************************************/

static int has_libfunc_name(const char* name) {
    int i;

    for (i = 0; i < LIBFUNC_COUNT; i++) {
        if (strcmp(name, g_libfuncs[i]) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

static const char* int_to_bool(int bool) {
    return bool ? "true" : "false";
}

static void update_current_line(unsigned int line) {

    assert(line);
    g_current_line = line;
}

static void update_current_function(Expression* func) {

    assert(func);

    epush(g_func_stack, g_current_function);

    g_current_function = func;
}

static void restore_current_function() {

    assert(current_scopespace() != SC_PROGRAM);

    g_current_function = epop(g_func_stack);
}

static Expression* current_function(void) {

    return g_current_function;
}

/*static void add_overload_check(iopcode op, Expression* expr1, Expression* expr2) {

    if (expr1->type == E_VAR || expr2->type == E_VAR) {
        return;
    }

    if (op != OP_ADD) {
        return;
    }

    if (expr1->type == E_CONSTNUM && expr2->type == E_CONSTNUM) {
        return;
    }

    if (expr1->type == E_CONSTSTRING && expr2->type == E_CONSTSTRING) {
        return;
    }

    yyerror("invalid operand in addition");
}*/

static int arithmetic_expression_check(iopcode op, Expression* expr) {

    expression_type e_type;
    const char* op_class = NULL;
    assert(expr);

    if (expr->type == E_VAR) {
        return TRUE;
    }

    switch (op) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
        case OP_UMINUS: {
            op_class = "arithmetic";
            break;
        }

        case OP_IF_EQ:
        case OP_IF_NOTEQ:
        case OP_IF_LESSEQ:
        case OP_IF_GREATEREQ:
        case OP_IF_LESS:
        case OP_IF_GREATER: {
            op_class = "relational";
            break;
        }

        default: {
            assert(0);
        }
    }

    if (op == OP_ADD && expr->type == E_CONSTSTRING) return TRUE;

    e_type = expr->type;
    switch (e_type) {
        case E_CONSTBOOL: {
            sprintf(g_genbuff, "invalid use of %s operator on constant boolean \'%s\'!", op_class, int_to_bool(expr->type));
            break;
        }

        case E_CONSTSTRING: {
            sprintf(g_genbuff, "invalid use of %s operator on constant string \"%s\"!", op_class, expr->const_string);
            break;
        }

        case E_LIBRARYFUNC:
        case E_PROGRAMFUNC: {
            sprintf(g_genbuff, "invalid use of %s operator on function \"%s\"!", op_class, expr->symbol->name);
            break;
        }

        case E_NEWTABLE: {
            sprintf(g_genbuff, "invalid use of %s operator on new table expression!", op_class);
            break;
        }

        case E_NIL: {
            sprintf(g_genbuff, "invalid use of %s operator on nil!", op_class);
            break;
        }

        default: {
            return TRUE;
        }
    }

    yyerror(g_genbuff);
    return FALSE;
}

static expression_type symbol_to_expr_type(SymTableEntry* symbol) {

    symbol_type s_type;
    expression_type e_type;

    assert(symbol);

    s_type = symbol->type;
    switch(s_type) {
        case S_GLOBAL:
        case S_LOCAL:
        case S_FORMAL: {
            e_type =  E_VAR;
            break;
        }
        case S_USERFUNC: {
            e_type =  E_PROGRAMFUNC;
            break;
        }
        case S_LIBFUNC: {
            e_type =  E_LIBRARYFUNC;
            break;
        }
        default: {
            assert(0);
        }
    }

    return e_type;
}

static expression_type const_token_to_expr_type(Token* token) {

    token_type t_type;
    expression_type e_type;

    assert(token);

    t_type = token->type;
    switch(t_type) {
        case T_CONSTNUM: {
            e_type = E_CONSTNUM;
            break;
        }

        case T_CONSTBOOL: {
            e_type = E_CONSTBOOL;
            break;
        }

        case T_CONSTSTRING: {
            e_type = E_CONSTSTRING;
            break;
        }

        case T_NIL: {
            e_type = E_NIL;
            break;
        }

        default: {
            assert(0);
        }
    }

    return e_type;
}

static char* new_anon_name(void) {

    static unsigned int func_count = 0;
    char* name;

    snprintf(g_genbuff, GENBUFF_LEN, "%d", func_count);

    name = mymalloc((2 + strlen(g_genbuff) + 1) * sizeof(char));
    name[0] = '\0';

    strncat(name, "$f", 3);
    strncat(name, g_genbuff, strlen(g_genbuff) + 1);

    ++func_count;

    return name;
}

static char* new_temp_name(void) {

    char* name = NULL;

    snprintf(g_genbuff, GENBUFF_LEN, "%d", g_temp_count);

    name = mymalloc((2 + strlen(g_genbuff) + 1) * sizeof(char));
    name[0] = '\0';

    strncat(name, "$t", 3);
    strncat(name, g_genbuff, strlen(g_genbuff) + 1);

    ++g_temp_count;

    strncpy(g_genbuff, name, GENBUFF_LEN);
    //free(name);

    return g_genbuff;
}

static SymTableEntry* new_temp(void) {

    SymTableEntry* entry = NULL;
    symbol_type st;
    char* name = NULL;

    name = new_temp_name();
    entry = symtable_lookup_in_scope(name, current_scope());

    if (entry == NULL) {

        if (current_scopespace() == SC_PROGRAM) {
            st = S_GLOBAL;
        } else if (current_scopespace() == SC_FORMAL) {
            st = S_FORMAL;
        } else {
            st = S_LOCAL;
        }

        entry = symtable_insert(name, st);
        entry->offset = current_offset();
        entry->space = current_scopespace();
        increment_offset();
    }

    return entry;
}

static void quad_expand(void) {

    Quad **new_quad_array = NULL;

    assert(g_icode_size == g_current_quad);

    new_quad_array = mymalloc(NEW_QUAD_SIZE);
    if (g_quad_array != NULL) {
        memcpy(new_quad_array, g_quad_array, CURR_QUAD_SIZE);
        free(g_quad_array);
    }

    g_quad_array = new_quad_array;
    g_icode_size += EXPAND_SIZE;
}

static void quad_insert(Quad* quad) {

    assert(quad);

    if (g_icode_size == g_current_quad) {
        quad_expand();
    }

    g_quad_array[g_current_quad++] = quad;
}

static void emit(iopcode op, Expression* result, Expression* arg1, Expression* arg2) {

    Quad* new_quad = NULL;

    assert(arg1);
    assert(result);

    new_quad = mymalloc(sizeof(Quad));


    new_quad->op = op;
    new_quad->arg1 = arg1;
    new_quad->arg2 = arg2;
    new_quad->result = result;
    new_quad->label = 0;
    new_quad->line = current_line();

    quad_insert(new_quad);
}

static void emit_conditional(iopcode op, Expression* arg1, Expression* arg2, unsigned int label) {

    Quad* new_quad = NULL;

    new_quad = mymalloc(sizeof(Quad));

    new_quad->op = op;
    new_quad->arg1 = arg1;
    new_quad->arg2 = arg2;
    new_quad->result = NULL;
    new_quad->label = label;
    new_quad->line = current_line();

    quad_insert(new_quad);
}

static void emit_newtable(iopcode op, Expression* arg) {

    Quad* new_quad = NULL;

    assert(arg);

    new_quad = mymalloc(sizeof(Quad));

    new_quad->op = op;
    new_quad->result = arg;
    new_quad->arg1 = NULL;
    new_quad->arg2 = NULL;
    new_quad->label = 0;
    new_quad->line = current_line();
    quad_insert(new_quad);
}

static void emit_func(iopcode op, Expression* expr) {

    Quad* new_quad = NULL;

    new_quad = mymalloc(sizeof(Quad));

    new_quad->op = op;
    new_quad->result = expr;
    new_quad->arg1 = NULL;
    new_quad->arg2 = NULL;
    new_quad->label = 0;
    new_quad->line = current_line();

    quad_insert(new_quad);
}

void emit_jump(unsigned int label) {

    Quad* new_quad = NULL;

    new_quad = mymalloc(sizeof(Quad));
    new_quad->op = OP_JUMP;
    new_quad->arg1 = NULL;
    new_quad->arg2 = NULL;
    new_quad->result = NULL;
    new_quad->label = label;
    new_quad->line = current_line() ? current_line() : 1;

    quad_insert(new_quad);
}

Expression* new_expression_constnum(double num) {

    Expression* e = new_expression(E_CONSTNUM);

    e->const_num = num;

    return e;
}

static Expression* new_expression_conststring(char* string) {

    Expression* e = new_expression(E_CONSTSTRING);

    e->const_string = mystrdup(string);

    return e;
}

static Expression* new_expression_temp(expression_type type) {

    Expression* e = new_expression(type);
    e->symbol = new_temp();

    return e;
}

static void backpatch(ListNode* head, unsigned int value) {

    assert(head);
    while (head != NULL) {
        g_quad_array[head->val - 1]->label = value;
        head = head->next;
    }
}

static void print_header(FILE* out) {
    PRINT_DELIM;
    fprintf(out, "brief: quads\ninput: %s\n", g_input_filename);
    PRINT_DELIM;
}


/********** SCOPE STUFF *******************************/

static void increment_scope(void) {

    ++g_current_scope;
}

static void decrement_scope(void) {

    assert(g_current_scope > 0);

    --g_current_scope;
}

unsigned int current_scope(void) {

    return g_current_scope;
}

void scopespace_enter(void) {

    ++g_scopespace_counter;
}

void scopespace_exit(void) {

    assert(g_scopespace_counter > 1);

    --g_scopespace_counter;
}

scopespace_type current_scopespace(void) {

    assert(g_scopespace_counter > 0);

    if (g_scopespace_counter == 1) {
        return SC_PROGRAM;
    } else if (g_scopespace_counter % 2 == 0) {
        return SC_FORMAL;
    } else {
        return SC_LOCAL;
    }
}

unsigned int current_offset(void) {

    switch (current_scopespace()) {
        case SC_PROGRAM: {
            return g_offset_program;
        }

        case SC_LOCAL: {
            return g_offset_local;
        }

        case SC_FORMAL: {
            return g_offset_formal;
        }

        default: {
            assert(0);
        }
    }
}

void increment_offset(void) {
    switch (current_scopespace()) {
        case SC_PROGRAM: {
            ++g_offset_program;
            break;
        }

        case SC_LOCAL: {
            ++g_offset_local;
            break;
        }

        case SC_FORMAL: {
            ++g_offset_formal;
            break;
        }

        default: {
            assert(0);
        }
    }
}

static void reset_formal_offset(void) {

    g_offset_formal = 0;
}

static void save_local_offset(void) {

    if (current_scopespace() != SC_PROGRAM) {
        push(g_local_stack, current_offset());
    }
}

static void reset_local_offset(void) {

    g_offset_local = 0;
}

static void restore_local_offset(void) {

    if (stack_is_empty(g_local_stack)) {
        g_offset_local = 0;
    } else {
        g_offset_local = pop(g_local_stack);
    }
}

/*****************************************************/


/********** LOOP STUFF *******************************/

static unsigned int current_loop_offset(void) {

    return g_loop_count;
}

static void increment_loop_offset(void) {

    ++g_loop_count;
}

static void decrement_loop_offset(void) {

    assert(g_loop_count > 0);

    --g_loop_count;
}

static void save_loop_offset(void) {

    push(g_loop_stack, g_loop_count);
}

static void reset_loop_offset(void) {

    g_loop_count = 0;
}

static void restore_loop_offset(void) {

    if (stack_is_empty(g_loop_stack)) {
        g_loop_count = 0;
    } else {
        g_loop_count = pop(g_loop_stack);
    }
}

/*****************************************************/

void init(void) {

    g_loop_count = 0;
    g_current_quad = 0;
    g_current_line = 0;
    g_current_scope = 0;
    g_libfuncs = NULL;
    g_scopespace_counter = 1;
    g_offset_formal = 0;
    g_offset_local = 0;
    g_offset_program = 0;
    g_quad_array = NULL;
    g_symtable = NULL;
    g_syntax_error = FALSE;
    g_temp_count = 0;
    g_icode_size = 0;
    g_blockend_line = 0;

    g_dummy = new_expression(E_VAR);

    g_unit = new_expression(E_CONSTNUM);
    g_unit->const_num = 1;

    g_minus = new_expression(E_CONSTNUM);
    g_minus->const_num = -1;

    g_true = new_expression(E_CONSTBOOL);
    g_true->const_bool = TRUE;

    g_false = new_expression(E_CONSTBOOL);
    g_false->const_bool = FALSE;

    g_local_stack = new_stack();
    g_loop_stack = new_stack();

    g_func_stack = new_estack();
    g_current_function = new_expression(E_PROGRAMFUNC);

    if (g_log) {
        g_icodeout = myfopen("logs/intermediate.txt", "w");
        g_symout = myfopen("logs/symbol_table.txt", "w");
    }

    symtable_create();
    init_lib();
    tcode_init();
}

void init_lib(void) {

    char* libfunc_names[] = {"print", "input", "objectmemberkeys", "objecttotalmembers", "objectcopy", "totalarguments", "argument", "typeof", "strtonum", "sqrt", "cos", "sin"};
    unsigned int i;

    assert(g_symtable);

    g_libfuncs = mymalloc(sizeof(char *) * LIBFUNC_COUNT);
    for (i = 0; i < LIBFUNC_COUNT; i++) {
        g_libfuncs[i] = libfunc_names[i];
        symtable_insert(g_libfuncs[i], S_LIBFUNC);
    }
}

void cleanup(void) {

    mem_cleanup();
    tcode_clean();

    if (g_syntax_error) {
        remove("logs/intermediate.txt");
        remove("logs/target.txt");
        remove("logs/symbol_table.txt");
        rmdir("logs");
    }

    if (g_log) {
        fclose(g_icodeout);
        fclose(g_symout);
    }
}

void yysuccess(const char* msg) {

    printf("\033[1m\033[32msuccess\033[0m: %s\n", msg);
}

void yynote(const char* msg) {

    fprintf(stderr, "\033[1m\033[34mnote\033[0m: %s\n", msg);
}

void yyerror(const char* msg) {

    fprintf(stderr, "\033[1m%s:%u: \033[31mcompile error\033[0m: %s\n", \
        g_input_filename, current_line(), msg);


    g_syntax_error = TRUE;
}

void yywarning(const char* msg) {

    fprintf(stderr, "\033[1m%s:%u: \033[35mwarning\033[0m: %s\n", \
            g_input_filename, g_current_line, msg);
}


unsigned int current_line(void) {

    return g_current_line;
}

unsigned int current_quad(void) {
    return g_current_quad;
}

unsigned int next_quad(void) {
    return g_current_quad + 1;
}

void reset_temp_count(void) {
    g_temp_count = 0;
}

void check_lvalue(expression_type e_type) {

    if (e_type == E_PROGRAMFUNC || e_type == E_LIBRARYFUNC) {
        yyerror("Using function as a numeric value!");
    }
}

Statement* handle_return(Expression* expr, unsigned int line) {

    update_current_line(line);

    if (current_scopespace() !=  SC_LOCAL) {
        yyerror("use of 'return' while not in a function");
    }

    emit_func(OP_RETURN, expr);

    return NULL;
}

void print_quads(void) {

    unsigned int i = 0;

    if (g_syntax_error || g_quad_array == NULL) {
        yynote("bytecode generation failed.");
        return;
    }

    print_header(g_icodeout);
    for (i = 0; i < g_current_quad; ++i) {
       fprintf(g_icodeout, "%4u: ", i + 1);
       printquad(g_icodeout, g_quad_array[i]);
    }
    fprintf(g_icodeout, "%s", "----------------------------------------\n");
}

Expression* new_expression(expression_type type) {

    Expression *expr = mymalloc(sizeof(Expression));

    expr->type = type;
    expr->symbol = NULL;
    expr->const_string = NULL;
    expr->next = NULL;
    expr->index = NULL;
    expr->truelist = NULL;
    expr->falselist = NULL;

    return expr;
}

Statement* new_statement(void) {

    Statement* stmt = NULL;

    stmt = mymalloc(sizeof(Statement));
    stmt->continuelist = NULL;
    stmt->breaklist = NULL;

    return stmt;
}

LoopStart* new_loopstart(Expression* test, unsigned int start) {

    LoopStart* loop = NULL;

    assert(test);

    loop = mymalloc(sizeof(LoopStart));
    loop->test = test;
    loop->start = start;

    return loop;
}

Expression* patch_bool(Expression* expr) {

    Expression* temp = NULL;
    assert(expr);

    if (expr->type == E_BOOLEXPR) {

        backpatch(expr->truelist, next_quad());
        backpatch(expr->falselist, next_quad() + 2);

        temp = new_expression(E_ASSIGNEXPR);
        temp->symbol = new_temp();

        emit(OP_ASSIGN, temp, g_true, NULL);
        emit_jump(next_quad() + 2);
        emit(OP_ASSIGN, temp, g_false, NULL);
    }

    return temp;
}

Statement* handle_expression_end(Expression* expr, unsigned int line) {

    assert(expr);

    update_current_line(line);
    reset_temp_count();

    patch_bool(expr);

    return NULL;
}

Expression* handle_indexedelem(Expression* index, Expression* element, unsigned int line) {

    assert(index);
    assert(element);

    element->index = index;

    return element;
}

Expression* handle_table_list(Expression* list, Expression* expr, unsigned int line) {

    Expression* temp = NULL;
    Expression* current = NULL;
    assert(list);

    update_current_line(line);

    if (expr == NULL) {
        temp = patch_bool(list);

        if (temp != NULL) {
            return temp;
        } else {
            return list;
        }
    }

    current = list;
    while (current->next != NULL) {
        current = current->next;
    }

    temp = patch_bool(expr);
    current->next = temp ? temp : expr;

    return list;
}

Expression* emit_iftableitem(Expression* e) {

    Expression* result = NULL;

    assert(e);

    if (e->type != E_TABLEITEM) {
        return e;
    }

    result = new_expression_temp(E_VAR);
    emit(OP_TABLEGETELEM, result, e, e->index);

    return result;
}

Expression* handle_op_assign(Expression* lvalue, Expression* expr, unsigned int line) {

    Expression* temp = NULL;

    assert(lvalue);
    assert(lvalue->symbol);
    assert(lvalue->type == E_VAR || lvalue->type == E_TABLEITEM);
    assert(expr);

    update_current_line(line);

    if (expr->type == E_BOOLEXPR) {
        backpatch(expr->truelist, next_quad());
        backpatch(expr->falselist, next_quad() + 2);

        temp = new_expression_temp(E_ASSIGNEXPR);

        emit(OP_ASSIGN, temp, g_true, NULL);
        emit_jump(next_quad() + 2);
        emit(OP_ASSIGN, temp, g_false, NULL);

        emit(OP_ASSIGN, lvalue, temp, NULL);

        temp = new_expression_temp(E_ASSIGNEXPR);

        emit(OP_ASSIGN, temp, lvalue, NULL);

        return temp;
    }

    if (lvalue->type == E_TABLEITEM) {

        emit(OP_TABLESETELEM, lvalue, lvalue->index, expr);

        temp = new_expression_temp(E_ASSIGNEXPR);
        emit(OP_TABLEGETELEM, temp, lvalue, lvalue->index);

        return temp;
    }

    temp = new_expression_temp(E_ASSIGNEXPR);

    emit(OP_ASSIGN, lvalue, expr, NULL);
    emit(OP_ASSIGN, temp, lvalue, NULL);


    return temp;
}

Expression* handle_op_arithm_binary(Expression* expr1, Expression* expr2, iopcode op, unsigned int line) {

    Expression* result_expr = NULL;

    assert(expr1);
    assert(expr2);

    update_current_line(line);

    arithmetic_expression_check(op, expr1);
    arithmetic_expression_check(op, expr2);

    //add_overload_check(op, expr1, expr2);

    result_expr = new_expression(E_ARITHMEXPR);
    result_expr->symbol = new_temp();
    emit(op, result_expr, expr1, expr2);

    return result_expr;
}

Expression* handle_op_eq(Expression* expr1, Expression* expr2, iopcode op, unsigned int line) {

    Expression* temp1 = NULL;
    Expression* temp2 = NULL;
    Expression* result_expr = NULL;

    assert(expr1);
    assert(expr2);

    assert(op == OP_IF_EQ || op == OP_IF_NOTEQ);

    update_current_line(line);

    temp1 = expr1->type == E_BOOLEXPR ? patch_bool(expr1) : expr1;
    temp2 = expr2->type == E_BOOLEXPR ? patch_bool(expr2) : expr2;

    result_expr = new_expression(E_BOOLEXPR);
    result_expr->truelist = list_make(next_quad());
    result_expr->falselist = list_make(next_quad() + 1);

    emit_conditional(op, temp1, temp2, 0);
    emit_jump(0);

    return result_expr;
}

Expression* handle_op_rel(Expression* expr1, Expression* expr2, iopcode op, unsigned int line) {

    Expression* result_expr = NULL;

    assert(expr1);
    assert(expr2);

    assert(op != OP_IF_EQ && op != OP_IF_NOTEQ);

    update_current_line(line);

    arithmetic_expression_check(op, expr1);
    arithmetic_expression_check(op, expr2);

    result_expr = new_expression(E_BOOLEXPR);
    result_expr->truelist = list_make(next_quad());
    result_expr->falselist = list_make(next_quad() + 1);

    emit_conditional(op, expr1, expr2, 0);
    emit_jump(0);

    return result_expr;
}

Expression* handle_op_bool(Expression* expr1, Expression* expr2, iopcode op, unsigned int M, unsigned int line) {

    Expression* result_expr = NULL;

    assert(expr1);
    assert(expr2);

    update_current_line(line);

    if (expr2->type != E_BOOLEXPR) {
        convert_to_bool(expr2);
    }

    result_expr = new_expression(E_BOOLEXPR);
    if (op == OP_AND) {
        backpatch(expr1->truelist, M);
        result_expr->truelist = expr2->truelist;
        result_expr->falselist = list_merge(expr1->falselist, expr2->falselist);
    } else if (op == OP_OR) {
        backpatch(expr1->falselist, M);
        result_expr->truelist = list_merge(expr1->truelist, expr2->truelist);
        result_expr->falselist = expr2->falselist;
    }

    return result_expr;
}

Expression* handle_op_not(Expression* expr, iopcode op, unsigned int line) {

    Expression* result = NULL;

    assert(expr);
    assert(op == OP_NOT);

    update_current_line(line);

    result = new_expression(E_BOOLEXPR);

    if (expr->type != E_BOOLEXPR) {
        result->falselist = list_make(next_quad());
        result->truelist = list_make(next_quad() + 1);

        emit_conditional(OP_IF_EQ, expr, g_true, 0);
        emit_jump(0);

        return result;
    }

    result->truelist = expr->falselist;
    result->falselist = expr->truelist;

    return result;
}

unsigned int convert_to_bool(Expression* expr) {

    assert(expr);

    if (expr->type != E_BOOLEXPR) {
        expr->truelist = list_make(next_quad());
        expr->falselist = list_make(next_quad() + 1);

        emit_conditional(OP_IF_EQ, expr, g_true, 0);
        emit_jump(0);
    }


    return next_quad();
}

Expression* handle_op_arithm_unary_pre(Expression* expr, iopcode op, unsigned int line) {

    Expression* temp = NULL;

    assert(expr);

    update_current_line(line);

    if (!arithmetic_expression_check(op, expr)) {
        return expr;
    }

    temp = new_expression(E_ARITHMEXPR);
    temp->symbol = new_temp();

    if (op == OP_UMINUS) {
        emit(OP_UMINUS, temp, expr, NULL);
    } else {
        emit(op, expr, expr, g_unit);
        emit(OP_ASSIGN, temp, expr, NULL);
    }

    return temp;
}

Expression* handle_op_arithm_unary_post(Expression* expr, iopcode op, unsigned int line) {

    Expression* temp = NULL;

    assert(expr);

    update_current_line(line);
    arithmetic_expression_check(op, expr);

    temp = new_expression(E_ARITHMEXPR);
    temp->symbol = new_temp();

    emit(OP_ASSIGN, temp, expr, NULL);
    emit(op, expr, expr, g_unit);

    return temp;
}

unsigned int handle_whilestart(void) {

    increment_loop_offset();

    return next_quad();
}

LoopStart* handle_for_prefix(Expression* test, unsigned int start) {

    Expression* result;

    assert(test);
    assert(start);

    increment_loop_offset();

    result = new_expression(E_BOOLEXPR);

    if (test->type != E_BOOLEXPR) {
        result = new_expression(E_BOOLEXPR);
        result->truelist = list_make(next_quad());
        result->falselist = list_make(next_quad() + 1);
        emit_conditional(OP_IF_EQ, test, g_true, 0);
        emit_jump(0);
    } else {
        result->truelist = test->truelist;
        result->falselist = test->falselist;
        result->symbol = test->symbol;
    }

    return new_loopstart(result, start);
}

Statement* handle_for(LoopStart* loopstart, unsigned int elist_start, unsigned int stmt_start, Statement* stmt, unsigned int loop_end) {

    ListNode* temp = NULL;

    assert(loopstart);

    decrement_loop_offset();
    backpatch(loopstart->test->truelist, stmt_start + 1);
    backpatch(loopstart->test->falselist, loop_end + 1);

    temp = list_make(stmt_start);
    backpatch(temp, loopstart->start);


    temp = list_make(loop_end);
    backpatch(temp, elist_start + 1);




    if (stmt && stmt->continuelist) {
        backpatch(stmt->continuelist, elist_start + 1   );
    }

    if (stmt && stmt->breaklist) {
        backpatch(stmt->breaklist, loop_end + 1);
    }

    return new_statement();
}

Expression* handle_whilecond(Expression* expr) {

    Expression* result = NULL;

    assert(expr);

    if (expr->type == E_BOOLEXPR) {
        backpatch(expr->truelist, next_quad());
        return expr;
    }

    result = new_expression(E_BOOLEXPR);
    result->falselist = list_make(next_quad() + 1);
    emit_conditional(OP_IF_EQ, expr, g_true, next_quad() + 2);
    emit_jump(0);

    return result;
}

void handle_program_end(Statement* stmts) {


}

Statement* handle_statement(Statement* stmts, Statement* stmt) {

    if (stmts == NULL && stmt == NULL) {
        return NULL;
    }

    if (stmts == NULL) {
        return stmt;
    }

    if (stmt == NULL) {
        return stmts;
    }

    stmts->breaklist = list_merge(stmts->breaklist, stmt->breaklist);
    stmts->continuelist = list_merge(stmts->continuelist, stmt->continuelist);

    //free(stmt->breaklist);
    //free(stmt->continuelist);
    //free(stmt);


    return stmts;
}

Statement* handle_loop_control(unsigned int type, unsigned int line) {

    Statement* stmt = NULL;

    assert(current_loop_offset() >= 0);
    assert(type == BREAK || type == CONTINUE);

    update_current_line(line);

    if (current_loop_offset() == 0) {
        snprintf(g_genbuff, GENBUFF_LEN, \
            "use of '%s' while not in a loop", type == BREAK ? "break" : "continue");
        yyerror(g_genbuff);
    }

    stmt = new_statement();
    if (type == BREAK) {
        stmt->breaklist = list_make(next_quad());
    } else {
        stmt->continuelist = list_make(next_quad());
    }

    emit_jump(0);

    return stmt;
}

Statement* handle_while_stmt(unsigned int loop_start_quad, Expression* expr, Statement* stmt) {

    assert(expr);
    assert(expr->falselist != NULL);

    assert(loop_start_quad < current_quad());

    decrement_loop_offset();
    emit_jump(loop_start_quad);
    backpatch(expr->falselist, next_quad());

    if (stmt != NULL && stmt->continuelist != NULL) {
        backpatch(stmt->continuelist, loop_start_quad);
    }

    if (stmt != NULL && stmt->breaklist != NULL) {
        backpatch(stmt->breaklist, next_quad());
    }

    return stmt;
}

Expression* handle_tablemake(Expression* list, unsigned int line) {

    Expression* current = NULL;
    Expression* t = NULL;
    size_t i;

    update_current_line(line);

    t = new_expression(E_NEWTABLE);
    t->symbol = new_temp();
    emit_newtable(OP_TABLECREATE, t);


    if (list == NULL) {
        return t;
    }

    for (i = 0, current = list; current != NULL; current = current->next) {
        if (current->index == NULL) {
            current->index = new_expression_constnum(i++);
        }
        emit(OP_TABLESETELEM, t, current->index, current);
    }

    return t;
}

Expression* handle_table_item_expr(Expression* lvalue, Expression* expr) {

    Expression* item = NULL;

    assert(lvalue);

    lvalue = emit_iftableitem(lvalue);

    item = new_expression(E_TABLEITEM);
    item->symbol = lvalue->symbol;
    item->index = expr;

    return item;
}

Expression* handle_table_item(Expression* lvalue, char* idname) {

    Expression* item = NULL;

    assert(lvalue);

    lvalue = emit_iftableitem(lvalue);

    item = new_expression(E_TABLEITEM);
    item->symbol = lvalue->symbol;
    item->index = new_expression_conststring(idname);

    return item;
}

Expression* handle_if_prefix(Expression* cond) {

    Expression* result = NULL;

    assert(cond);

    if (cond->type == E_BOOLEXPR) {
        backpatch(cond->truelist, next_quad());
        return cond;
    }

    result = new_expression(E_BOOLEXPR);
    result->falselist = list_make(next_quad() + 1);
    emit_conditional(OP_IF_EQ, cond, g_true, next_quad() + 2);
    emit_jump(0);

    return result;
}

Statement* handle_if_stmt(Expression* cond, Statement* stmt) {

    assert(cond);

    backpatch(cond->falselist, next_quad());

    return stmt;
}

Statement* handle_if_else_stmt(Statement* stmt1, unsigned int quadtopatch, Statement* stmt2) {

    assert(quadtopatch < current_quad());
    assert(g_quad_array[quadtopatch]->label == 0);

    g_quad_array[quadtopatch]->label = next_quad();

    if (stmt1 != NULL) {

        stmt1->breaklist = list_merge(stmt1->breaklist, stmt2->breaklist);
        stmt1->continuelist = list_merge(stmt1->continuelist, stmt2->continuelist);

        if (stmt2 != NULL) {
            //free(stmt2->breaklist);
            //free(stmt2->continuelist);
        }
    }

    return stmt1;
}

unsigned int patch_if_else(Expression* cond) {

    unsigned int result = current_quad();

    assert(cond);
    assert(cond->type == E_BOOLEXPR);

    backpatch(cond->falselist, next_quad() + 1);

    emit_jump(0);

    return result;
}

Expression* handle_const_token(Token* const_token) {

    Expression* expr = NULL;
    expression_type type;

    type = const_token_to_expr_type(const_token);
    expr = new_expression(type);
    switch(type) {
        case E_CONSTNUM: {
            expr->const_num = const_token->const_num;
            break;
        }

        case E_CONSTSTRING: {
            expr->const_string = const_token->const_string;
            break;
        }

        case E_CONSTBOOL: {
            expr->const_bool = const_token->const_bool;
            break;
        }

        case E_NIL: {
            expr->const_string = "nil";
            break;
        }

        default: {
            assert(0);
        }
    }

    return expr;
}

Expression* handle_id_local(char* name, unsigned int line) {

    SymTableEntry *entry = NULL;
    Expression* expr = NULL;

    update_current_line(line);

    /* lookup id name in symbol table in current scope only */
    entry = symtable_lookup_in_scope(name, current_scope());
    if (entry == NULL) {
        if (current_scope() == GLOBAL_SCOPE) {
            entry = symtable_insert(name, S_GLOBAL);
            entry->space = current_scopespace();
            entry->offset = current_offset();
            increment_offset();
            print_new_id(entry);
        } else if (!has_libfunc_name(name)) {
            entry = symtable_insert(name, S_LOCAL);
            print_new_id(entry);        /* the current scope (??) */
        } else {
            sprintf(g_genbuff, "Variable \"%s\" shadows a library function", name);
            yyerror(g_genbuff);
            //free(name);
            return NULL;
        }
    } else {
        print_found_id(entry);
    }

    expr = new_expression(symbol_to_expr_type(entry));
    expr->symbol = entry;



    return expr;
}

Expression* handle_id_global(char* name, unsigned int line) {

    SymTableEntry *entry = NULL;
    Expression* expr = NULL;

    update_current_line(line);

    entry = symtable_lookup_in_scope(name, GLOBAL_SCOPE);
    if (entry == NULL) {
        snprintf(g_genbuff, GENBUFF_LEN, "No global variable \"%s\" exists", name);
        yyerror(g_genbuff);
        return NULL;
    } else {
        print_found_id(entry);
    }

    expr = new_expression(symbol_to_expr_type(entry));
    expr->symbol = entry;

    return expr;
}

Expression* handle_id_plain(char* name, unsigned int line) {

    SymTableEntry *entry = NULL;
    Expression *expr  = NULL;

    update_current_line(line);

    entry = symtable_lookup(name);
    if (entry == NULL) {
        entry = symtable_insert(name, current_scope() == 0 ? S_GLOBAL : S_LOCAL);
        entry->space = current_scopespace();
        entry->offset = current_offset();
        increment_offset();
        print_new_id(entry);
    } else if (entry->space != SC_PROGRAM && current_function()->symbol->scope >= entry->scope) {
        snprintf(g_genbuff, GENBUFF_LEN, "cannot access '%s' inside '%s'", name, current_function()->symbol->name);
        yyerror(g_genbuff);
        return g_dummy;
    }


    print_found_id(entry);

    expr = new_expression(symbol_to_expr_type(entry));
    expr->symbol = entry;


    return expr;
}

void handle_id_formal(char* name, unsigned int line) {

    SymTableEntry *new_entry = NULL;
    SymTableEntry* entry = NULL;

    assert(current_scopespace() == SC_FORMAL);

    update_current_line(line);

    if (has_libfunc_name(name)) {
        sprintf(g_genbuff, "Formal variable \"%s\" shadows a library function", name);
        yyerror(g_genbuff);
        return;
    }

    entry = symtable_lookup_in_scope(name, current_scope());
    if (entry != NULL) {
        sprintf(g_genbuff, "Formal variable \"%s\" is already defined at line: %u", name, entry->line);
        yyerror(g_genbuff);
        return;
    }

    new_entry = symtable_insert(name, S_FORMAL);
    new_entry->space = current_scopespace();
    new_entry->offset= current_offset();
    increment_offset();

    print_new_id(new_entry);
}

void block_enter(void) {

    increment_scope();
}

void block_exit(unsigned int line) {

    symtable_hide(current_scope());
    decrement_scope();

    g_blockend_line = line;
}

Call* handle_normcall(Expression* elist) {

    Call* suffix = mymalloc(sizeof(Call));

    suffix->name = NULL;
    suffix->elist = elist;
    suffix->is_method = FALSE;

    return suffix;
}

Call* handle_methodcall(char* name, Expression* elist) {

    Call* suffix = mymalloc(sizeof(Call));

    suffix->name = mystrdup(name);
    suffix->elist = elist;
    suffix->is_method = TRUE;

    return suffix;
}

Expression* handle_call(Expression* lvalue, Call* suffix, unsigned int line) {

    Expression* func = NULL;

    update_current_line(line);

    lvalue = emit_iftableitem(lvalue);
    if (suffix->is_method) {
        func = emit_iftableitem(handle_table_item(lvalue, suffix->name));
        lvalue->next = suffix->elist;
        suffix->elist = lvalue;
    } else {
        func = lvalue;
    }

    return handle_call_simple(func, suffix->elist, line);
}

Expression* handle_call_simple(Expression* lvalue, Expression* elist, unsigned int line) {

    EStack* revstack = new_estack();
    Expression* result = NULL;
    Expression* func = NULL;

    update_current_line(line);

    func = emit_iftableitem(lvalue);

    while (elist) {
        epush(revstack, elist);
        elist = elist->next;
    }

    while (estack_get_size(revstack) > 0) {
        emit_func(OP_PARAM, epop(revstack));
    }

    emit_func(OP_CALL, func);
    result = new_expression_temp(E_VAR);
    emit_func(OP_GETRETVAL, result);

    //free(revstack);

    return result;
}

char* handle_funcname(char* name) {

    return name ? mystrdup(name) : new_anon_name();
}

Expression* handle_funcprefix(char* name, unsigned int line) {

    Expression* new_expr = NULL;
    SymTableEntry* entry = NULL;
    SymTableEntry* new_entry = NULL;

    update_current_line(line);

    if (has_libfunc_name(name)) {
        sprintf(g_genbuff, "user function \"%s\" shadows library function", name);
        yyerror(g_genbuff);
        return NULL;
    }

    entry = symtable_lookup_in_scope(name, current_scope());
    if (entry != NULL) {
        sprintf(g_genbuff, "symbol \"%s\" is already defined at line: %u", name, entry->line);
        yyerror(g_genbuff);
        return NULL;
    }

    new_entry = symtable_insert(name, S_USERFUNC);

    new_expr = new_expression(symbol_to_expr_type(new_entry));
    new_expr->symbol = new_entry;
    new_expr->symbol->iaddress = next_quad() + 1;
    new_expr->falselist = list_make(next_quad());

    update_current_function(new_expr);

    emit_jump(0);
    emit_func(OP_FUCNSTART, new_expr);

    save_loop_offset();
    reset_loop_offset();

    save_local_offset();
    reset_local_offset();

    reset_formal_offset();

    scopespace_enter();     /* enter 'formal' scopespace                */
    increment_scope();      /* increment scope for formal arguments     */

    print_new_id(new_expr->symbol);

    return new_expr;
}

void handle_funcargs(void) {

    /* the ensuing block rule will increment the scope again*/
    /* so, because we already are +1 scope from the funcdef , we decrement here */
    decrement_scope();

    /* enter 'local' scopespace */
    scopespace_enter();
}

unsigned int handle_funcbody(void) {

    unsigned int local_count;

    assert(current_scopespace() == SC_LOCAL);

    local_count = current_offset();

    scopespace_exit();

    return local_count;
}

Expression* handle_funcdef(Expression* func, unsigned int local_count) {

    assert(func);

    update_current_line(g_blockend_line);
    restore_current_function();

    scopespace_exit();

    func->symbol->local_count = local_count;
    backpatch(func->falselist, next_quad() + 1);

    restore_local_offset();
    restore_loop_offset();

    emit_func(OP_FUNCEND, func);

    return func;
}
