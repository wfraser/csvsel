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

#include "queryparse.h"

static growbuf* SELECTED_COLUMNS;
static compound** ROOT_CONDITION;

extern int query_debug;

static int query_parse(void);
extern int query_lex();
static void query_error();

int queryparse(const char* query, size_t query_length, growbuf* selected_columns, compound** root_condition)
{
    SELECTED_COLUMNS = selected_columns;
    ROOT_CONDITION = root_condition;

    //TODO: need to feed the query string to the bison parser.
    // perhaps via a pipe??? :[

    return query_parse();
}

compound* new_compound()
{
    compound* c = (compound*)malloc(sizeof(compound));
    if (NULL == c) {
        fprintf(stderr, "malloc failed in query parser!\n");
        exit(EX_OSERR);
    }
    memset(c, 0, sizeof(compound));
    return c;
}

%}

%union {
    long      num;
    char*     str;
    rval      rval;
    compound* compound;
}

%token TOK_SELECT TOK_WHERE TOK_EQ TOK_GT TOK_LT TOK_GTE TOK_LTE TOK_AND TOK_OR TOK_NOT TOK_LPAREN TOK_RPAREN TOK_COMMA TOK_ERROR

%token <num> TOK_NUMBER
%token <num> TOK_COLUMN
%token <str> TOK_STRING

%type <rval> Rvalue
%type <compound> Clause

%error-verbose

%%

Start : TOK_SELECT Columns TOK_WHERE Conditions
      | TOK_SELECT Columns
{
}

Columns : Columns TOK_COMMA TOK_COLUMN {
            size_t col = $3;
            growbuf_append(SELECTED_COLUMNS, &col, sizeof(size_t));
            printf("I haz the last column: %ld\n", $3);
          }
        | TOK_COLUMN {
            size_t col = $1;
            growbuf_append(SELECTED_COLUMNS, &col, sizeof(size_t));
            printf("I haz a column: %ld\n", $1);
          }
        | { 
            size_t max = SIZE_MAX;
            growbuf_append(SELECTED_COLUMNS, &max, sizeof(size_t));
            printf("I haz no columns!\n");
          }
;

Conditions  : Clause {
                printf("root condition is type %d\n", $1->oper);
                *ROOT_CONDITION = $1;
            }
;

Clause  : TOK_COLUMN TOK_EQ Rvalue {
            $$ = new_compound();

            $$->simple.column = $1;
            $$->simple.oper = TOK_EQ;
            $$->simple.rval = $3;

            $$->oper = OPER_SIMPLE;
        }
        | TOK_COLUMN TOK_GT Rvalue {
            $$ = new_compound();

            $$->simple.column = $1;
            $$->simple.oper = TOK_GT;
            $$->simple.rval = $3;

            $$->oper = OPER_SIMPLE;
        }
        | TOK_COLUMN TOK_LT Rvalue {
            $$ = new_compound();

            $$->simple.column = $1;
            $$->simple.oper = TOK_LT;
            $$->simple.rval = $3;

            $$->oper = OPER_SIMPLE;
        }
        | TOK_COLUMN TOK_GTE Rvalue {
            $$ = new_compound();

            $$->simple.column = $1;
            $$->simple.oper = TOK_GTE;
            $$->simple.rval = $3;

            $$->oper = OPER_SIMPLE; 
        }
        | TOK_COLUMN TOK_LTE Rvalue {
            $$ = new_compound();

            $$->simple.column = $1;
            $$->simple.oper = TOK_LTE;
            $$->simple.rval = $3;

            $$->oper = OPER_SIMPLE;
        }
        | Clause TOK_AND Clause {
            $$ = new_compound();
            $$->left = $1;
            $$->right = $3;
            $$->oper = OPER_AND;
        }
        | Clause TOK_OR Clause {
            $$ = new_compound();
            $$->left = $1;
            $$->right = $3;
            $$->oper = OPER_OR;
        }
        | TOK_NOT Clause {
            $$ = new_compound();
            $$->left = $2;
            $$->oper = OPER_NOT;
        }
        | TOK_LPAREN Clause TOK_RPAREN {
            $$ = $2;
        }
;

Rvalue  : TOK_COLUMN {
            printf("rvalue is column: %ld\n", $1);
            $$.col = $1;
            $$.is_col = true;
        }
        | TOK_STRING {
            printf("rvalue is string: %s\n", $1);
            $$.str = $1;
            $$.is_str = true;
        }
        | TOK_NUMBER {
            printf("rvalue is number: %ld\n", $1);
            $$.num = $1;
            $$.is_num = true;
        }
;

%%

void query_error(const char* s)
{
    // intentionally empty
}


