%{

/**
 * CSVSEL Query Parser
 *
 * by William R. Fraser, 10/22/2011
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "growbuf.h"

#include "query.h"

static growbuf* SELECTED_COLUMNS;
static growbuf* CONDITIONS;

static int query_parse(void);
static int query_lex();
static void query_error();

int queryparse(const char* query, size_t query_length, growbuf* selected_columns, growbuf* conditions)
{
    SELECTED_COLUMNS = selected_columns;
    CONDITIONS       = conditions;

    return query_parse();
}

#define YYSTYPE YYSTYPE
typedef union
{
    long  num;
    char* str;
} YYSTYPE;

%}

%token TOK_SELECT TOK_WHERE TOK_PREFIX TOK_EQ TOK_GT TOK_LT TOK_GTE TOK_LTE TOK_AND TOK_OR TOK_NOT TOK_LPAREN TOK_RPAREN TOK_NUM TOK_COMMA TOK_STAR TOK_STRING

%error-verbose

%%

Start : TOK_SELECT Columns TOK_WHERE Conditions
      | TOK_SELECT Columns
{
}

Columns : Columns TOK_COMMA Column {
            size_t col = $3.num;
            growbuf_append(SELECTED_COLUMNS, &col, sizeof(size_t));
          }
        | Column {
            size_t col = $1.num;
            growbuf_append(SELECTED_COLUMNS, &col, sizeof(size_t));
          }
        | { 
            size_t max = SIZE_MAX;
            growbuf_append(SELECTED_COLUMNS, &max, sizeof(size_t));
          }
;

Column : TOK_PREFIX TOK_NUM {
            $0 = $2;
        }
;

Conditions : Conditions Condition {}
           | Condition {}
;

Condition : Condition Clause {}
          | Clause {}
;

Clause  : Column TOK_EQ Rvalue
        | Column TOK_GT Rvalue
        | Column TOK_LT Rvalue
        | Column TOK_GTE Rvalue
        | Column TOK_LTE Rvalue
        | Clause TOK_AND Clause
        | Clause TOK_OR Clause
        | TOK_NOT Clause
        | TOK_LPAREN Clause TOK_RPAREN
;

Rvalue  : Column
        | TOK_STRING
        | TOK_NUM
;

%%

void query_error(const char* s)
{
    // intentionally empty
}


