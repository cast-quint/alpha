/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * avm_util.h
 */

#ifndef AVM_UTIL_H
#define AVM_UTIL_H

#include "avm_table.h"

#define AVM_SAVED_TOPSP_OFFSET     1
#define AVM_SAVED_TOP_OFFSET       2
#define AVM_SAVED_PC_OFFSET        3
#define AVM_SAVED_ARG_COUNT_OFFSET 4
#define AVM_CALLENV_SIZE           4
#define AVM_STACK_SIZE             4096
#define AVM_PC_ENDING              avm_inst_size
#define AVM_WIPEOUT(m)             memset(&(m), 0, sizeof(m))

typedef struct Function Function;
typedef struct Instruction Instruction;
typedef struct Arg Arg;

typedef enum memcell_t {
    M_NUMBER,
    M_STRING,
    M_BOOL,
    M_TABLE,
    M_USERFUNC,
    M_LIBFUNC,
    M_NIL,
    M_UNDEF
} memcell_t;

typedef struct Memcell {
    memcell_t type;
    union {
        double        val_num;
        char*         val_string;
        unsigned char val_bool;
        Table*        val_table;
        unsigned int  val_func;
        char*         val_lib;
    };
} Memcell;

extern Instruction** avm_instructions;
extern unsigned int avm_inst_size;
extern Instruction* avm_curr_inst;


extern Memcell  avm_stack[AVM_STACK_SIZE];
extern Memcell* erx;    /* return register  */
extern Memcell* eax;    /* general register */
extern Memcell* ebx;    /* general register */
extern Memcell* ecx;    /* general register */

extern const unsigned int glob;
extern unsigned int top;
extern unsigned int topsp;
extern unsigned int pc;

extern int execution_done;
extern unsigned int total_actual_args;

extern const char* avm_string_types[8];

extern FILE* bytecode_file;
extern char bytecode_filename[256];

char* avm_print_type(Memcell* c);
void avm_error(const char* msg);
void avm_warning(const char* msg);
void avm_clear(Memcell* c);
void avm_execute(void);
void assert_memcell(Memcell* c);
int avm_is_int(double num);

const char* avm_tostring(Memcell* mem);

void avm_push_env_val(unsigned int value);
void avm_save_env(void);
void avm_decrement_top(void);

Function* avm_get_func(unsigned int offset);
Memcell* avm_translate_to_memcell(Arg* arg, Memcell* reg);


#endif /* AVM_UTIL_H */