/**
 * CSVSEL Query Lexer
 *
 * by William R. Fraser, 10/23/2011
 */

%{
#include <stdio.h>
#include <string.h>
#include "queryparse.h"
#include "queryparse.tab.h"
#include "growbuf.h"

growbuf* string_buf = NULL;
%}

%option yylineno
%option noyywrap

COLUMN          %[0-9]+
INTEGER         -?[0-9]+
FLOAT           -?[0-9]+\.[0-9]*
WHITESPACE      [ \t\r\n]
IDENTIFIER      [a-zA-Z_][a-zA-Z0-9_-]+

%x STRING

%%

{WHITESPACE}    { /* do nothing */ }

"select"        { return TOK_SELECT; }
"where"         { return TOK_WHERE; }
"contains"      { return TOK_CONTAINS; }
"order"         { return TOK_ORDER; }
"by"            { return TOK_BY; }
"asc"           { return TOK_ASCENDING; }
"ascending"     { return TOK_ASCENDING; }
"desc"          { return TOK_DESCENDING; }
"descending"    { return TOK_DESCENDING; }
"="             { return TOK_EQ; }
"!="            { return TOK_NEQ; }
">"             { return TOK_GT; }
"<"             { return TOK_LT; }
">="            { return TOK_GTE; }
"<="            { return TOK_LTE; }
"and"           { return TOK_AND; }
"or"            { return TOK_OR; }
"not"           { return TOK_NOT; }
"("             { return TOK_LPAREN; }
")"             { return TOK_RPAREN; }
"-"             { return TOK_DASH; }
","             { return TOK_COMMA; }

".int"          { return TOK_CONV_NUM; }
".float"        { return TOK_CONV_DBL; }
".string"       { return TOK_CONV_STR; }

{COLUMN}        { 
                    query_lval.col = atoi(yytext + 1) - 1;
                    return TOK_COLUMN; 
                }

\%%             {
                    query_lval.special = SPECIAL_NUMCOLS;
                    return TOK_SPECIAL;
                }

%#              {
                    query_lval.special = SPECIAL_ROWNUM;
                    return TOK_SPECIAL;
                }

{INTEGER}       { 
                    query_lval.num = atoi(yytext);
                    return TOK_INTEGER; 
                }

{FLOAT}         {
                    query_lval.dbl = strtod(yytext, NULL);
                    return TOK_FLOAT;
                }

{IDENTIFIER}    {
                    query_lval.str = strdup(yytext);
                    return TOK_IDENTIFIER;
                }

\"              { 
                    BEGIN(STRING);
                    string_buf = growbuf_create(32);
                }

<STRING>\"      {
                    BEGIN(INITIAL);
                    char nul = '\0';
                    growbuf_append(string_buf, &nul, 1);
                    query_lval.str = string_buf->buf;
                    free(string_buf);
                    string_buf = NULL;
                    return TOK_STRING;
                }

<STRING>\n      {
                    BEGIN(INITIAL);
                    growbuf_free(string_buf);
                    string_buf = NULL;
                    return TOK_ERROR;
                }

<STRING>\\n     { growbuf_append_byte(string_buf, '\n'); }
<STRING>\\t     { growbuf_append_byte(string_buf, '\t'); }
<STRING>\\r     { growbuf_append_byte(string_buf, '\r'); }
<STRING>\\b     { growbuf_append_byte(string_buf, '\b'); }
<STRING>\\f     { growbuf_append_byte(string_buf, '\f'); }

<STRING>\\(.|\n) {
                    growbuf_append(string_buf, &(query_text[1]), 1);
                }

<STRING>[^\\\n\"]+ {
                    growbuf_append(string_buf, query_text, strlen(query_text));
                }

\0              { /* nothing */ }

.               {
                    fprintf(stderr, "illegal '%s' in input\n", yytext);
                    return TOK_ERROR;
                }

%%
