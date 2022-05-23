/*
 * Dimitrios Koropoulis, 3967
 * CS340 - Spring 2021
 * tcode.h
 */

#ifndef TCODE_H
#define TCODE_H

#include "util.h"

#define MAX_STACK_SIZE 512
#define MAGIC_NUMBER   340213967

extern Expression* g_minus;
extern Quad** g_quad_array;
extern char g_genbuff[GENBUFF_LEN];
extern const char* g_input_filename;
extern int g_log;

typedef struct Expression Expression;

typedef enum {
    A_LABEL     = 0,
    A_GLOBAL    = 1,
    A_FORMAL    = 2,
    A_LOCAL     = 3,
    A_NUM       = 4,
    A_STRING    = 5,
    A_BOOL      = 6,
    A_NIL       = 7,
    A_USERFUNC  = 8,
    A_LIBFUNC   = 9,
    A_RETVAL    = 10,
    A_DUMMY     = 11
} arg_type;

typedef enum {
    VOP_ASSIGN,

    VOP_ADD,
    VOP_SUB,
    VOP_MUL,
    VOP_DIV,
    VOP_MOD,

    VOP_JEQ,
    VOP_JNE,
    VOP_JGT,
    VOP_JLT,
    VOP_JGE,
    VOP_JLE,
    VOP_JUMP,

    VOP_NEWTABLE,
    VOP_TABLEGETELEM,
    VOP_TABLESETELEM,

    VOP_ENTERFUNC,
    VOP_EXITFUNC,
    VOP_CALLFUNC,
    VOP_PUSHARG,

    VOP_NOP
} vopcode;

typedef struct Arg {
    arg_type type;
    unsigned int val;
} Arg;

typedef struct Instruction {
    vopcode op;
    struct Arg* result;
    struct Arg* arg1;
    struct Arg* arg2;
    unsigned int line;
} Instruction;

typedef struct IncompleteJump {
    unsigned int quad_num;
    unsigned int jump_label;
    struct IncompleteJump* next;
} IncompleteJump;

typedef struct IncompleteReturn {
    unsigned int inst_num;
    struct IncompleteReturn* next;
} IncompleteReturn;

typedef struct ReturnStack {
    IncompleteReturn* items[MAX_STACK_SIZE];
    size_t top;
} ReturnStack;


typedef struct Function {
    unsigned int taddress;
    unsigned int local_size;
    const char* name;
} Function;

void  add_jump(unsigned int icode_num, unsigned int tcode_num);
void  patch_jumps(void);

void  emit_inst(vopcode op, Arg* result, Arg* arg1, Arg* arg2, unsigned int line);
void  generate_tcode(void);
Arg*  make_arg(Expression* e);
void  tcode_init(void);
void  tcode_clean(void);
char* make_bytefilename(void);
void  write_bytecode(void);
void  print_bytecode(void);


char* print_vopcode(vopcode op);
char* print_arg(Arg* arg);

#endif /* TCODE_H */
