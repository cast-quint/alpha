/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * parse.y
 */

%{
    #include <stdio.h>
    #include <assert.h>

    #include "memory-management.h"
    #include "symtable.h"
    #include "util.h"

    extern int yylex(void);
    extern FILE* yyin;
%}

%language "C"
%defines
%define parse.error verbose

%union {
    Token        token;
    Expression*  expr;
    Statement*   stmt;
    LoopStart*   loopstart;
    Call*        call;
    char*        name;
    unsigned int num;
}

%start program

%type <expr> assign_expr
%type <expr> const
%type <expr> elist
%type <expr> expression
%type <expr> if_prefix
%type <expr> indexed
%type <expr> indexedelem
%type <expr> lvalue
%type <expr> table_item
%type <expr> call_item
%type <expr> primary
%type <expr> tablemake
%type <expr> term
%type <expr> whilecond
%type <expr> funcprefix
%type <expr> funcdef
%type <expr> call

%type <call> call_suffix
%type <call> norm_call
%type <call> method_call

%type <stmt> block
%type <stmt> for_statement
%type <stmt> if_statement
%type <stmt> statement
%type <stmt> statements
%type <stmt> while_statement
%type <stmt> return_statement

%type <num> E
%type <num> I
%type <num> M
%type <num> N
%type <num> N1
%type <num> whilestart
%type <num> funcbody

%type <name> funcname

%type <loopstart> for_prefix

%token <token> CONST_FLOAT
%token <token> CONST_INT
%token <token> ID
%token <token> STRING

%token <token> KEYWORD_FALSE    "false"
%token <token> KEYWORD_NIL      "nil"
%token <token> KEYWORD_TRUE     "true"
%token <token> KEYWORD_AND      "and"
%token KEYWORD_BREAK            "break"
%token KEYWORD_CONTINUE         "continue"
%token KEYWORD_ELSE             "else"
%token KEYWORD_FOR              "for"
%token KEYWORD_FUNCTION         "function"
%token KEYWORD_IF               "if"
%token KEYWORD_LOCAL            "local"
%token KEYWORD_NOT              "not"
%token KEYWORD_OR               "or"
%token KEYWORD_RETURN           "return"
%token KEYWORD_WHILE            "while"
%token OPERATOR_EQ              "=="
%token OPERATOR_GE              ">="
%token OPERATOR_LE              "<="
%token OPERATOR_MINUS_MINUS     "--"
%token OPERATOR_NE              "!="
%token OPERATOR_PLUS_PLUS       "++"
%token PUNCTUATOR_DOUBLE_COLON  "::"
%token PUNCTUATOR_DOUBLE_PERIOD ".."

%precedence '='
%left       "or"
%left       "and"
%left       "==" "!="
%nonassoc   '>' ">=" '<' "<="
%left       '+' '-'
%left       '*' '/' '%'
%precedence "not"
%precedence OPERATOR_UMINUS
%precedence NO_ELSE
%precedence "else"

%%

program:              %empty
                    | statements            { handle_program_end($1); }
                    ;

statements:           statement             { $$ = $1;                        }
                    | statements statement  { $$ = handle_statement($1, $2); }
                    ;

statement:            expression ';'    { $$ = handle_expression_end($1, @1.first_line);     }
                    | if_statement      { $$ = $1;                                           }
                    | while_statement   { $$ = $1;                                           }
                    | for_statement     { $$ = $1;                                           }
                    | return_statement  { $$ = $1;                                           }
                    | "break" ';'       { $$ = handle_loop_control(BREAK, @1.first_line);    }
                    | "continue" ';'    { $$ = handle_loop_control(CONTINUE, @1.first_line); }
                    | block             { $$ = $1;                                           }
                    | funcdef           { $$ = NULL; }
                    | error ';'         {;}
                    | error '}'         {;}
                    | error ')'         {;}
                    | ';'               { $$ = NULL; }
                    ;

expression:           term                          { $$ = $1;                                                              }
                    | assign_expr                   { $$ = $1;                                                              }
                    | expression '+'   expression   { $$ = handle_op_arithm_binary($1, $3, OP_ADD,          @1.first_line); }
                    | expression '-'   expression   { $$ = handle_op_arithm_binary($1, $3, OP_SUB,          @1.first_line); }
                    | expression '*'   expression   { $$ = handle_op_arithm_binary($1, $3, OP_MUL,          @1.first_line); }
                    | expression '/'   expression   { $$ = handle_op_arithm_binary($1, $3, OP_DIV,          @1.first_line); }
                    | expression '%'   expression   { $$ = handle_op_arithm_binary($1, $3, OP_MOD,          @1.first_line); }
                    | expression '>'   expression   { $$ = handle_op_rel          ($1, $3, OP_IF_GREATER,   @1.first_line); }
                    | expression ">="  expression   { $$ = handle_op_rel          ($1, $3, OP_IF_GREATEREQ, @1.first_line); }
                    | expression '<'   expression   { $$ = handle_op_rel          ($1, $3, OP_IF_LESS,      @1.first_line); }
                    | expression "<="  expression   { $$ = handle_op_rel          ($1, $3, OP_IF_LESSEQ,    @1.first_line); }
                    | expression "=="  expression   { $$ = handle_op_eq           ($1, $3, OP_IF_EQ,        @1.first_line); }
                    | expression "!="  expression   { $$ = handle_op_eq           ($1, $3, OP_IF_NOTEQ,     @1.first_line); }
                    | expression "and" M expression { $$ = handle_op_bool         ($1, $4, OP_AND, $3,      @1.first_line); }
                    | expression "or"  M expression { $$ = handle_op_bool         ($1, $4, OP_OR,  $3,      @1.first_line); }
                    ;

M:                    %empty { $$ = convert_to_bool($<expr>-1); };

assign_expr:          lvalue '=' expression { $$ = handle_op_assign($1, $3, @1.first_line); };


term:                 primary                               { $$ = $1;                                                         }
                    | '(' expression ')'                    { $$ = $2;                                                         }
                    | "not" expression                      { $$ = handle_op_not              ($2,  OP_NOT,    @2.first_line); }
                    | '-' expression %prec OPERATOR_UMINUS  { $$ = handle_op_arithm_unary_pre ($2,  OP_UMINUS, @2.first_line); }
                    | "++" lvalue                           { $$ = handle_op_arithm_unary_pre ($2,  OP_ADD,    @2.first_line); }
                    | "--" lvalue                           { $$ = handle_op_arithm_unary_pre ($2,  OP_SUB,    @2.first_line); }
                    | lvalue "++"                           { $$ = handle_op_arithm_unary_post($1,  OP_ADD,    @1.first_line); }
                    | lvalue "--"                           { $$ = handle_op_arithm_unary_post($1,  OP_SUB,    @1.first_line); }
                    ;

primary:              lvalue          { $$ = emit_iftableitem($1); }
                    | call            { $$ = $1; }
                    | tablemake       { $$ = $1; }
                    | '(' funcdef ')' { $$ = $2; }
                    | const           { $$ = $1; }
                    ;

tablemake:            '[' elist   ']' { $$ = handle_tablemake($2, @1.first_line); }
                    | '[' indexed ']' { $$ = handle_tablemake($2, @1.first_line); }
                    ;

const:                CONST_INT     { $$ = handle_const_token(&$1); }
                    | CONST_FLOAT   { $$ = handle_const_token(&$1); }
                    | STRING        { $$ = handle_const_token(&$1); }
                    | "nil"         { $$ = handle_const_token(&$1); }
                    | "true"        { $$ = handle_const_token(&$1); }
                    | "false"       { $$ = handle_const_token(&$1); }
                    ;


lvalue:                       ID { $$ = handle_id_plain ($1.id_name, @1.first_line); }
                    | "local" ID { $$ = handle_id_local ($2.id_name, @1.first_line); }
                    | "::"    ID { $$ = handle_id_global($2.id_name, @1.first_line); }
                    | table_item { $$ = $1;                                          }
                    | call_item  { $$ = $1;                                          }
                    ;

table_item:           lvalue '.' ID             { $$ = handle_table_item($1, $3.id_name); }
                    | lvalue '[' expression ']' { $$ = handle_table_item_expr($1, $3);    }
                    ;

call_item:            call   '.' ID             { $$ = handle_table_item($1, $3.id_name); }
                    | call   '[' expression ']' { $$ = handle_table_item_expr($1, $3);    }
                    ;


call:                 lvalue call_suffix            { $$ = handle_call       ($1, $2, @1.first_line); }
                    | call '(' elist ')'            { $$ = handle_call_simple($1, $3, @1.first_line); }
                    | '(' funcdef ')' '(' elist ')' { $$ = handle_call_simple($2, $5, @2.first_line); }
                    ;

call_suffix:          norm_call   { $$ = $1; }
                    | method_call { $$ = $1; }
                    ;

norm_call:            '(' elist ')'         { $$ = handle_normcall($2);                };

method_call:          ".." ID '(' elist ')' { $$ = handle_methodcall($2.id_name,  $4); };

elist:                %empty                    { $$ = NULL;                                       }
                    | expression                { $$ = handle_table_list($1, NULL, @1.first_line); }
                    | elist ',' expression      { $$ = handle_table_list($1, $3,   @1.first_line); }
                    ;

indexed:              indexedelem               { $$ = handle_table_list($1, NULL, @1.first_line); }
                    | indexed ',' indexedelem   { $$ = handle_table_list($1, $3,   @1.first_line); }
                    ;

indexedelem:          '{' expression ':' expression '}'  { $$ = handle_indexedelem($2, $4, @1.first_line); };

idlist:               %empty        { ; }
                    | idlist ',' ID { handle_id_formal($3.id_name, @3.first_line); }
                    |            ID { handle_id_formal($1.id_name, @1.first_line); }
                    ;

brace_open:         '{' { block_enter(); };

brace_close:        '}' { block_exit(@1.first_line);  };

block:                brace_open statements brace_close { $$ = $2;   }
                    | brace_open            brace_close { $$ = NULL; }
                    ;

funcname:             ID     { $$ = handle_funcname($1.id_name); }
                    | %empty { $$ = handle_funcname(NULL);       }
                    ;

funcprefix:           "function" funcname { $$ = handle_funcprefix($2, @1.first_line); };

funcargs:             '(' idlist ')' { handle_funcargs(); };

funcbody:             block { $$ = handle_funcbody(); };

funcdef:              funcprefix funcargs funcbody { $$ = handle_funcdef($1, $3); };


if_prefix:            "if" '(' expression ')'  { $$ = handle_if_prefix($3); };

if_statement:         if_prefix statement %prec NO_ELSE      { $$ = handle_if_stmt($1, $2);      }
                    | if_prefix statement "else" I statement { $$ = handle_if_else_stmt($2, $4, $5); }
                    ;

I:                    %empty { $$ = patch_if_else($<expr>-2); };


whilestart:           "while"                        { $$ = handle_whilestart();            };

whilecond:            '(' expression ')'             { $$ = handle_whilecond($2);           };

while_statement:      whilestart whilecond statement { $$ = handle_while_stmt($1, $2, $3);  };


N:                    %empty { $$ = next_quad(); emit_jump(0); };

N1:                   %empty { $$ = current_quad(); };

E:                    %empty { $$ = next_quad(); };

for_prefix:           "for" '(' elist ';' E expression ';'  { $$ = handle_for_prefix($6, $5); };

for_statement:        for_prefix N1 elist ')' N statement N { $$ = handle_for($1, $2, $5, $6, $7); };


return_statement:     "return" expression ';' {  $$ = handle_return($2, @1.first_line);   }
                    | "return" ';'            {  $$ = handle_return(NULL, @1.first_line); }
                    ;

%%
