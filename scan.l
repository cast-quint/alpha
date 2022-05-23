/*
 * Dimitrios Koropoulis 3967
 * CS340 - Spring 2021
 * scan.l
 */

%{
    #include "util.h"
    #include "parse.h"
    #define MAX_STR_CONST 256

    #define YY_USER_ACTION { yylloc.first_line = yylineno; }
    #define YY_CLEAN_UP    { yy_delete_buffer(YY_CURRENT_BUFFER); yylex_destroy(); }

    unsigned int comment_stack = 0;
    char * string_buf_ptr;
    char err_msg[128];

	unsigned int g_token_count;
	unsigned int g_comment_line_start;
	unsigned int g_comment_line_end;
	char string_buf[MAX_STR_CONST];

    extern char* mystrdup(const char *);
    extern unsigned int g_current_line;
%}

%option yylineno
%option noyywrap
%option nounput
%option noinput

WHITESPACE          [ \t\n]+
DIGIT               [0-9]
LETTER              [a-zA-Z]
ID                  {LETTER}(_|{LETTER}|{DIGIT})*
CONST_INT           {DIGIT}{DIGIT}*
CONST_FLOAT         {CONST_INT}"."{CONST_INT}
COMMENT_LINE        \/\/.*

%x STRING_PARSING
%x COMMENT_BLOCK

%%

"/*"                    {
                            ++comment_stack;
                            BEGIN(COMMENT_BLOCK);
                        }
<COMMENT_BLOCK>{
"/*"                    { ++comment_stack; }
"*/"                    {
                            --comment_stack;
                            if (comment_stack == 0) {
                                BEGIN(INITIAL);
                            }

                        }
<<EOF>>                 {;
                            g_current_line = yylineno;
                            yyerror("Unterminated comment!");
                            YY_CLEAN_UP
                            return 0;
                        }
.|\n                    { ; }
}

\"                      { string_buf_ptr = string_buf; BEGIN(STRING_PARSING); }
<STRING_PARSING>{
\"                      {
                            BEGIN(INITIAL);
                            *string_buf_ptr = '\0';
                            yylval.token.type = T_CONSTSTRING;
                            yylval.token.const_string = mystrdup(string_buf);
                            return STRING;
                        }
\\n                     { *string_buf_ptr++ = '\n'; }
\\t                     { *string_buf_ptr++ = '\t'; }
\\\\                    { *string_buf_ptr++ = '\\'; }
\\\"                    { *string_buf_ptr++ = '"'; }
\\.                     {
                            sprintf(err_msg, "Bad escape character: '%s'", yytext);
                            g_current_line = yylineno;
                            yywarning(err_msg);
                        }
<<EOF>>                 {
                            g_current_line = yylineno;
                            yyerror("Reached EOF while parsing string!");
                            YY_CLEAN_UP
                            return 0;
                        }
[^\\\n\"]+              {
                            char *yptr = yytext;
                            while ( *yptr ) {
                                *string_buf_ptr++ = *yptr++;
                            }
                        }
}

{COMMENT_LINE}          { ; }
{WHITESPACE}            { ; }

"if"                    { return KEYWORD_IF; }
"else"                  { return KEYWORD_ELSE; }
"while"                 { return KEYWORD_WHILE; }
"for"                   { return KEYWORD_FOR; }
"function"              { return KEYWORD_FUNCTION; }
"return"                { return KEYWORD_RETURN; }
"break"                 { return KEYWORD_BREAK; }
"continue"              { return KEYWORD_CONTINUE; }
"and"                   { return KEYWORD_AND; }
"not"                   { return KEYWORD_NOT; }
"or"                    { return KEYWORD_OR; }
"local"                 { return KEYWORD_LOCAL; }
"true"                  {
							yylval.token.type = T_CONSTBOOL;
							yylval.token.const_bool = 1;
							return KEYWORD_TRUE;
						}
"false"                 {
							yylval.token.type = T_CONSTBOOL;
                            yylval.token.const_bool = 0;
							return KEYWORD_FALSE;
						}
"nil"                   {
                            yylval.token.type = T_NIL;
                            return KEYWORD_NIL;
                        }

"="                     { return '='; }
"+"                     { return '+'; }
"-"                     { return '-'; }
"*"                     { return '*'; }
"/"                     { return '/'; }
"%"                     { return '%'; }
"=="                    { return OPERATOR_EQ; }
"!="                    { return OPERATOR_NE; }
"++"                    { return OPERATOR_PLUS_PLUS; }
"--"                    { return OPERATOR_MINUS_MINUS; }
">"                     { return '>'; }
"<"                     { return '<'; }
">="                    { return OPERATOR_GE; }
"<="                    { return OPERATOR_LE; }

"{"                     { return '{'; }
"}"                     { return '}'; }
"["                     { return '['; }
"]"                     { return ']'; }
"("                     { return '('; }
")"                     { return ')'; }
";"                     { return ';'; }
","                     { return ','; }
":"                     { return ':'; }
"::"                    { return PUNCTUATOR_DOUBLE_COLON; }
"."                     { return '.'; }
".."                    { return PUNCTUATOR_DOUBLE_PERIOD; }

{CONST_INT}             {
							yylval.token.type = T_CONSTNUM;
							yylval.token.const_num = atoi(yytext);
							return CONST_INT;
						}

{CONST_FLOAT}           {
							yylval.token.type = T_CONSTNUM;
							yylval.token.const_num = atof(yytext);
							return CONST_FLOAT;
						}

{ID}                    {
							yylval.token.type = T_ID;
							yylval.token.id_name = mystrdup(yytext);
							return ID;
						}

<<EOF>>                 {
                            YY_CLEAN_UP
                            return 0;
                        }

.                       {
                            g_current_line = yylineno;
                            sprintf(err_msg, "Undefined character! [ %s ]", yytext);
                            yyerror(err_msg);
                            YY_CLEAN_UP
                            return 0;
                        }

%%
