/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * util.h
 */

#ifndef UTIL_H
#define UTIL_H

#define MAX_SCOPE_COUNT     256
#define MAX_FORMAL_COUNT    256
#define GENBUFF_LEN         512

#define TRUE                1
#define FALSE               0
#define BREAK               668269
#define CONTINUE            677978

#include <stdio.h>
#include <stdlib.h>
#include "symtable.h"

typedef struct Stack Stack;
typedef struct EStack EStack;
typedef struct ListNode ListNode;
typedef struct FreeTable FreeTable;
typedef struct FreeTableBucket FreeTableBucket;

typedef enum {
    T_ID,
    T_CONSTNUM,
    T_CONSTBOOL,
    T_CONSTSTRING,
    T_NIL
} token_type;

typedef enum {
    E_VAR,
    E_TABLEITEM,
    E_NEWTABLE,
    E_PROGRAMFUNC,
    E_LIBRARYFUNC,

    E_ARITHMEXPR,
    E_BOOLEXPR,
    E_ASSIGNEXPR,

    E_CONSTNUM,
    E_CONSTBOOL,
    E_CONSTSTRING,

    E_NIL,
    E_DUMMY
} expression_type;

typedef enum {
    OP_ASSIGN,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_UMINUS,

    OP_IF_EQ,
    OP_IF_NOTEQ,
    OP_IF_GREATER,
    OP_IF_LESS,
    OP_IF_GREATEREQ,
    OP_IF_LESSEQ,
    OP_JUMP,

    OP_TABLECREATE,
    OP_TABLEGETELEM,
    OP_TABLESETELEM,

    OP_CALL,
    OP_FUCNSTART,
    OP_FUNCEND,
    OP_PARAM,
    OP_RETURN,
    OP_GETRETVAL,

    OP_AND,
    OP_OR,
    OP_NOT
} iopcode;

typedef struct Token {
    token_type type;
    union {
        char* id_name;
        double const_num;
        char* const_string;
        int const_bool;
    };
} Token;

typedef struct Expression {
    expression_type type;
    SymTableEntry* symbol;                  /* if an ID, the symbol table entry  */
    union {
        double const_num;                   /* if a const number, the value      */
        char* const_string;                 /* if a const string, the value      */
        int const_bool;                     /* if a const bool, the value        */
    };
    struct Expression* next;                /* if elist, the next in the list     */
    struct Expression* index;               /* if table, the expression index     */
    struct ListNode* truelist;              /* if boolexpr, it's truelist         */
    struct ListNode* falselist;             /* if boolexpr, it's falselist        */
} Expression;

typedef struct Call {
    char* name;
    Expression* elist;
    int is_method;
} Call;

typedef struct LoopStart {
    struct Expression* test;
    unsigned int start;
} LoopStart;

typedef struct Statement {
    struct ListNode* breaklist;
    struct ListNode* continuelist;
} Statement;

typedef struct Quad {
    iopcode op;
    struct Expression* arg1;
    struct Expression* arg2;
    struct Expression* result;
    unsigned int label;
    unsigned int line;

    unsigned int iaddress;
    unsigned int taddress;
} Quad;

/**************************/

unsigned int    current_scope(void);

void            scopespace_enter(void);
void            scopespace_exit(void);

scopespace_type current_scopespace(void);

unsigned int    current_offset(void);
void            increment_offset(void);

Call*           handle_normcall(Expression* elist);
Call*           handle_methodcall(char* name, Expression* elist);

Expression*     emit_iftableitem(Expression* e);
Expression*     handle_const_token(Token* const_token);
Expression*     handle_elist(Expression* elist, Expression* expr);
Expression*     handle_funcprefix(char* name, unsigned int line);
Expression*     handle_id_global(char* name, unsigned int line);
Expression*     handle_id_local(char* name, unsigned int line);
Expression*     handle_id_plain(char* name, unsigned int line);
Expression*     handle_if_prefix(Expression* expr);
Expression*     handle_indexedelem(Expression* index, Expression* element, unsigned int line);
Expression*     handle_member_item(Expression* lvalue, char* idname);
Expression*     handle_op_arithm_binary(Expression* expr1, Expression* expr2, iopcode op, unsigned int line);
Expression*     handle_op_arithm_unary_post(Expression* expr, iopcode op, unsigned int line);
Expression*     handle_op_arithm_unary_pre(Expression* expr, iopcode op, unsigned int line);
Expression*     handle_op_assign(Expression* lvalue, Expression* arg, unsigned int line);
Expression*     handle_op_bool(Expression* expr1, Expression* expr2, iopcode op, unsigned int M, unsigned int line);
Expression*     handle_op_eq(Expression* expr1, Expression* expr2, iopcode op, unsigned int line);
Expression*     handle_op_not(Expression* expr, iopcode op, unsigned int line);
Expression*     handle_op_rel(Expression* expr1, Expression* expr2, iopcode op, unsigned int line);
Expression*     handle_table_item(Expression* lvalue, char* name);
Expression*     handle_table_item_expr(Expression* lvalue, Expression* expr);
Expression*     handle_table_list(Expression* list, Expression* expr, unsigned int line);
Expression*     handle_tablemake(Expression* elist, unsigned int line);
Expression*     handle_whilecond(Expression* expr);
Expression*     new_expression(expression_type type);
Expression*     handle_funcdef(Expression* func, unsigned int local_count);
Expression*     handle_call_simple(Expression* lvalue, Expression* elist, unsigned int line);
Expression*     handle_call(Expression* lvalue, Call* suffix, unsigned int line);
Expression*     new_expression_constnum(double num);

LoopStart*      handle_for_prefix(Expression* test, unsigned int start);

Statement*      handle_return(Expression* expr, unsigned int line);
Statement*      handle_expression_end(Expression* expr, unsigned int line);
Statement*      handle_for(LoopStart* loopstart, unsigned int elist_start, unsigned int stmt_start, Statement* stmt, unsigned int loop_end);
Statement*      handle_if_else_stmt(Statement* stmt1, unsigned int quadtopatch, Statement* stmt2);
Statement*      handle_if_stmt(Expression* cond, Statement* stmt);
Statement*      handle_loop_control(unsigned int type, unsigned int line);
Statement*      handle_statement(Statement* stmts, Statement* stmt);
Statement*      handle_while_stmt(unsigned int label, Expression* expr, Statement* stmt);
Statement*      new_statement(void);

void            handle_id_formal(char* name, unsigned int line);

char*           handle_funcname(char* name);

expression_type get_const_expr_type(Token* token);
expression_type get_symbol_expr_type(SymTableEntry* symbol);

unsigned int    convert_to_bool(Expression* expr);
unsigned int    current_line(void);

unsigned int    handle_whilestart(void);
unsigned int    patch_if_else(Expression* cond);
unsigned int    next_quad(void);
unsigned int    current_quad(void);
unsigned int    handle_funcbody(void);

void            check_lvalue(expression_type e_type);
void            emit_jump(unsigned int label);
void            expand_quad_array(void);
void            global_free(void);
void            block_enter(void);
void            block_exit(unsigned int line);
void            handle_program_end(Statement* statements);
void            handle_funcargs(void);
void            init(void);
void            init_lib(void);
void            cleanup(void);
void            quad_free(void);
void            print_quads(void);
void            reset_temp_count(void);

void            yyerror(const char* msg);
void            yynote(const char* msg);
void            yywarning(const char* msg);
void            yysuccess(const char* msg);


#endif /* UTIL_H */

