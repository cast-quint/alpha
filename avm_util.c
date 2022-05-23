/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * avm_util.c
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "memory-management.h"
#include "tcode.h"
#include "avm_util.h"
#include "avm_inst.h"

FILE* bytecode_file = NULL;
char bytecode_filename[256];

static unsigned int  avm_global_count = 0;

static char**        avm_strtable = NULL;
static unsigned int  avm_strtable_size = 0;

static double*       avm_numtable = NULL;
static unsigned int  avm_numtable_size = 0;

static Function**    avm_functable = NULL;
static unsigned int  avm_functable_size = 0;

static char**        avm_libtable = NULL;
static unsigned int  avm_libtable_size = 0;

static unsigned int  avm_curr_line = 0;

const char* avm_string_types[8] = {
    "number",
    "string",
    "boolean",
    "table",
    "userfunction",
    "libraryfunction",
    "nil",
    "undefined"
};


int execution_done = 0;

Instruction** avm_instructions = NULL;
unsigned int  avm_inst_size = 0;
Instruction*  avm_curr_inst = 0;

Memcell  avm_stack[AVM_STACK_SIZE];
Memcell* erx;
Memcell* eax;
Memcell* ebx;

const unsigned int glob  = AVM_STACK_SIZE - 1;
unsigned int       top   = AVM_STACK_SIZE - 1;
unsigned int       topsp = AVM_STACK_SIZE - 1;

unsigned int pc = 0;

static double avm_get_num(unsigned int offset) {

    assert(offset < avm_numtable_size);

    return avm_numtable[offset];
}

static char* avm_get_string(unsigned int offset) {

    assert(offset < avm_strtable_size);

    return strdup(avm_strtable[offset]);
}

static char* avm_get_lib(unsigned int offset) {

    assert(offset < avm_libtable_size);

    return avm_libtable[offset];
}

static void avm_init_stack(void) {

    size_t i;

    for (i = 0; i < AVM_STACK_SIZE; i++) {
        AVM_WIPEOUT(avm_stack[i]);
        avm_stack[i].type = M_UNDEF;
    }
}

static void avm_load_bytecode(void) {

    unsigned int magic;
    unsigned long int size;
    unsigned int i;


    /* Magic */
    fread(&magic, sizeof(unsigned int), 1, bytecode_file);
    //printf("magic: %u\n\n", magic);
    if (magic != MAGIC_NUMBER) {
        avm_error("invalid bytecode file");
        exit(EXIT_FAILURE);
    }


    /* Global count */
    fread(&avm_global_count, sizeof(unsigned int), 1, bytecode_file);
    //printf("Global count: %u\n\n", avm_global_count);


    /* Strings */
    fread(&avm_strtable_size, sizeof(unsigned int), 1, bytecode_file);
    avm_strtable = mymalloc(avm_strtable_size * sizeof(char *));
    //printf("String count: %u\n", avm_strtable_size);

    for (i = 0; i < avm_strtable_size; i++) {
        fread(&size, sizeof(unsigned long int), 1, bytecode_file);
        avm_strtable[i] = mymalloc((size + 1) * sizeof(char));
        fread(avm_strtable[i], sizeof(char), size + 1, bytecode_file);
        //printf("%u: size = %lu, name = \"%s\"\n", i + 1, size, avm_strtable[i]);
    }

    /* Numbers */

    fread(&avm_numtable_size, sizeof(unsigned int), 1, bytecode_file);
    //printf("\nNumber count: %u\n", avm_numtable_size);
    avm_numtable = mymalloc(avm_numtable_size * sizeof(double));
    for (i = 0; i < avm_numtable_size; i++) {
        fread(&avm_numtable[i], sizeof(double), 1, bytecode_file);
        //printf("  %u: %.2lf\n", i + 1, avm_numtable[i]);
    }

    /* Functions */
    fread(&avm_functable_size, sizeof(unsigned int), 1, bytecode_file);
    //printf("\nFunction count: %u\n", avm_functable_size);
    avm_functable = mymalloc(avm_functable_size * sizeof(Function *));
    for (i = 0; i < avm_functable_size; i++) {
        avm_functable[i] = mymalloc(sizeof(Function));

        fread(&avm_functable[i]->taddress, sizeof(unsigned int), 1, bytecode_file);
        fread(&avm_functable[i]->local_size, sizeof(unsigned int), 1, bytecode_file);

        fread(&size, sizeof(unsigned long int), 1, bytecode_file);
        avm_functable[i]->name = mymalloc((size + 1) * sizeof(char));
        fread((char*)avm_functable[i]->name, sizeof(char), size + 1, bytecode_file);

        //printf("%u: %s, addr: %u, local size: %u\n", i + 1, avm_functable[i]->name, avm_functable[i]->taddress, avm_functable[i]->local_size);
    }

   /* Library Functions */
    fread(&avm_libtable_size, sizeof(unsigned int), 1, bytecode_file);
    //printf("lib count: %u\n", avm_libtable_size);
    avm_libtable = mymalloc(avm_libtable_size * sizeof(char *));
    for (i = 0; i < avm_libtable_size; i++) {
        fread(&size, sizeof(unsigned long int), 1, bytecode_file);
        avm_libtable[i] = mymalloc((size + 1) * sizeof(char));
        fread(avm_libtable[i], sizeof(char), size + 1, bytecode_file);
        //printf("%u: size = %lu, name = %s\n", i + 1, size, avm_libtable[i]);
    }

    /* Code */
    fread(&avm_inst_size, sizeof(unsigned int), 1, bytecode_file);
    avm_instructions = mymalloc(avm_inst_size * sizeof(Instruction *));
    for (i = 0; i < avm_inst_size; i++) {
        avm_instructions[i] = mymalloc(sizeof(Instruction));
        avm_instructions[i]->result = mymalloc(sizeof(Arg));
        avm_instructions[i]->arg1 = mymalloc(sizeof(Arg));
        avm_instructions[i]->arg2 = mymalloc(sizeof(Arg));

        fread(&avm_instructions[i]->op, sizeof(vopcode), 1, bytecode_file);
        fread(avm_instructions[i]->result, sizeof(Arg), 1, bytecode_file);
        fread(avm_instructions[i]->arg1, sizeof(Arg), 1, bytecode_file);
        fread(avm_instructions[i]->arg2, sizeof(Arg), 1, bytecode_file);
        fread(&avm_instructions[i]->line, sizeof(unsigned int), 1, bytecode_file);
        /*printf("%4u: %12s %s %s %s [%u]\n", i, print_vopcode(avm_instructions[i]->op), \
            print_arg(avm_instructions[i]->result), print_arg(avm_instructions[i]->arg1), print_arg(avm_instructions[i]->arg2), avm_instructions[i]->line);*/
    }



    fclose(bytecode_file);
}

static void avm_clear_string(Memcell* c) {

    assert(c->val_string);
    free(c->val_string);

    c->type = M_UNDEF;
}

static void avm_clear_table(Memcell* c) {

    assert(c->val_table);

    avm_decrement_ref(c->val_table);
}

typedef void (*memclear_func_t)(Memcell*);

static memclear_func_t autoclear[] = {
    0,                   /* M_NUMBER   */
    avm_clear_string,    /* M_STRING   */
    0,                   /* M_BOOL     */
    avm_clear_table,     /* M_TABLE    */
    0,                   /* M_USERFUNC */
    0,                   /* M_LIBFUNC  */
    0,                   /* M_NIL      */
    0                    /* M_UNDEF    */
};

static void (*autoexec[])(Instruction*) = {
    execute_assign,         /* ASSIGN       */
    execute_arithm,         /* ADD          */
    execute_arithm,         /* SUB          */
    execute_arithm,         /* MUL          */
    execute_arithm,         /* DIV          */
    execute_arithm,         /* MOD          */
    execute_jeq,            /* JEQ          */
    execute_jne,            /* JNE          */
    execute_relop,          /* JGT          */
    execute_relop,          /* JLT          */
    execute_relop,          /* JGE          */
    execute_relop,          /* JLE          */
    execute_jump,           /* JUMP         */
    execute_newtable,       /* NEWTABLE     */
    execute_tablegetelem,   /* TABLEGETELEM */
    execute_tablesetelem,   /* TABLESETELEM */
    execute_enterfunc,      /* ENTERFUNC    */
    execute_exitfunc,       /* EXITFUNC     */
    execute_callfunc,       /* CALLFUNC     */
    execute_pusharg,        /* PUSHARG      */
    execute_nop             /* NOP          */
};

void execute_nop(Instruction* inst) {
    return;
}

static void execute_cycle(void) {

    unsigned int pc_temp;

    if (execution_done) {
        return;
    }

    if (pc == AVM_PC_ENDING) {
        execution_done = 1;
        return;
    }

    assert(pc < AVM_PC_ENDING);

    avm_curr_inst = *(avm_instructions + pc);
    avm_curr_line = avm_curr_inst->line;

    assert(avm_curr_inst->op >= VOP_ASSIGN && avm_curr_inst->op <= VOP_NOP);

    pc_temp = pc;
    autoexec[avm_curr_inst->op](avm_curr_inst);

    if (pc_temp == pc) {
        ++pc;
    }
}

Function* avm_get_func(unsigned int offset) {

    assert(offset < avm_functable_size);

    return avm_functable[offset];
}

int avm_is_int(double num) {

    return (num - (int)num == 0);
}

void assert_memcell(Memcell* c) {

    assert(c);
    assert(((c <= &avm_stack[AVM_STACK_SIZE - 1]) && (c > &avm_stack[top])) || c == erx);
}

char* avm_print_type(Memcell* c) {

    assert(c);

    switch (c->type) {

        case M_NUMBER: {
            return "number";
        }

        case M_STRING: {
            return "string";
        }

        case M_BOOL: {
            return "bool";
        }

        case M_TABLE: {
            return "table";
        }

        case M_USERFUNC:
        case M_LIBFUNC: {
            return "function";
        }

        case M_NIL: {
            return "nil";
        }

        case M_UNDEF: {
            return "undefined";
        }

        default: {
            assert(0);
        }
    }
}

static const char* parse_table(Table* table) {

    memcell_t t;
    char temp[2048];
    char buff[2048];
    unsigned int i = 0;
    unsigned int count = 0;
    unsigned int type_total = 0;
    int first_print = 1;
    TableBucket* current = NULL;

    buff[0] = '[';
    buff[1] = ' ';
    buff[2] = '\0';

    for (t = 0; t < AVM_VALID_TYPES_COUNT; ++t) {
        count = 0;
        type_total = table->indexed_total[t];
        if (type_total == 0) {
            continue;
        }

        if (first_print) {
            first_print = 0;
        } else {
            strncat(buff, ", ", strlen(buff));
        }

        for (i = 0; i < AVM_HASHTABLE_SIZE; ++i) {
            current = table->indexed[t][i];
            while (current) {
                if (current->value->type != M_NIL) {
                    ++count;
                    snprintf(temp, 2048, "{ %s : %s }%s",   \
                        avm_tostring(current->key),         \
                        avm_tostring(current->value),       \
                        (count < type_total ? ", " : ""));

                    strncat(buff, temp, 2048 - strlen(buff));
                }
                current = current->next;
            }
        }
    }

    strncat(buff, " ]", 2048 - strlen(buff));


    return mystrdup(buff);
}

const char* avm_tostring(Memcell* mem) {

    char buff[1024];

    assert(mem);

    switch (mem->type) {

        case M_NUMBER: {
            avm_is_int(mem->val_num) ? snprintf(buff, 1024, "%d", (int)mem->val_num) : snprintf(buff, 1024, "%.3lf", mem->val_num);

            return mystrdup(buff);
        }

        case M_STRING: {
            return mem->val_string;
        }

        case M_BOOL: {
            return mem->val_bool ? "true" : "false";
        }

        case M_TABLE: {
            return parse_table(mem->val_table);
        }

        case M_USERFUNC: {
            snprintf(buff, 1024, "user function: %u", mem->val_func);
            return mystrdup(buff);
        }

        case M_LIBFUNC: {
            snprintf(buff, 1024, "library function: %s", mem->val_lib);
            return mystrdup(buff);
        }

        case M_NIL: {
            return "nil";
        }

        case M_UNDEF: {
            return "undef";
        }

        default: {
            assert(0);
        }
    }
}

void avm_error(const char* msg) {

    fprintf(stderr, "\033[1m%s:%u: \033[31merror\033[0m: %s\n", \
        bytecode_filename, avm_curr_line, msg);

    execution_done = 1;
}

void avm_warning(const char* msg) {

    fprintf(stderr, "\033[1m%s:%u: \033[35mwarning\033[0m: %s\n", \
            bytecode_filename, avm_curr_line, msg);
}

static void avm_init(void) {

    avm_init_stack();
    avm_load_bytecode();

    erx = mymalloc(sizeof(Memcell));
    erx->type = M_UNDEF;

    eax = mymalloc(sizeof(Memcell));
    eax->type = M_UNDEF;

    ebx = mymalloc(sizeof(Memcell));
    ebx->type = M_UNDEF;

    top = top - avm_global_count;
}

static void avm_cleanup(void) {

    size_t i;

    avm_clear(erx);
    avm_clear(eax);
    avm_clear(ebx);

    for (i = 0; i < AVM_STACK_SIZE; i++) {
        avm_clear(&avm_stack[i]);
    }

    mem_cleanup();
}

void avm_clear(Memcell* c) {

    memclear_func_t f;

    if (c->type != M_UNDEF) {
        f = autoclear[c->type];
        if (f) {
            (*f)(c);
        }
        c->type = M_UNDEF;
    }
}

Memcell* avm_translate_to_memcell(Arg* arg, Memcell* reg) {

    //unsigned int i;
    switch (arg->type) {

        /* VARIABLES */
        case A_GLOBAL: {
            /*printf(" == STACK GLOB -- (PC->%u) ==\n", pc);
            for (i = 0; i < avm_global_count; i++) {
                printf(" == [%u]: %s == \n", AVM_STACK_SIZE - 1 - i, avm_string_types[avm_stack[AVM_STACK_SIZE - 1 - i].type]);
            }*/

            return &avm_stack[glob - arg->val];
        }

        case A_FORMAL: {
            return &avm_stack[topsp + AVM_CALLENV_SIZE + 1 + arg->val];
        }

        case A_LOCAL: {
            return &avm_stack[topsp - arg->val];
        }

        case A_RETVAL: {
            return erx;
        }

        /* CONSTANTS */

        case A_BOOL: {
            assert(reg);
            reg->type = M_BOOL;
            reg->val_bool = arg->val;
            break;
        }

        case A_NUM: {
            assert(reg);
            reg->type = M_NUMBER;
            reg->val_num = avm_get_num(arg->val);
            break;
        }

        case A_STRING: {
            assert(reg);
            reg->type = M_STRING;
            reg->val_string = avm_get_string(arg->val);
            break;
        }

        case A_NIL: {
            assert(reg);
            reg->type = M_NIL;
            break;
        }

        /* FUNCTIONS */

        case A_USERFUNC: {
            assert(reg);
            reg->type = M_USERFUNC;
            reg->val_func = avm_get_func(arg->val)->taddress;
            break;
        }

        case A_LIBFUNC: {
            assert(reg);
            reg->type = M_LIBFUNC;
            reg->val_lib = avm_get_lib(arg->val);
            break;
        }

        default: {
            assert(0);
        }
    }


    return reg;
}

void avm_execute(void) {

    avm_init();

    while (!execution_done) {
        execute_cycle();
    }

    avm_cleanup();
}


