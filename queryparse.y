%{

/**
 * CSVSEL Query Parser
 *
 * by William R. Fraser, 10/22/2011
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include "growbuf.h"

#include "queryparse.h"

static growbuf* SELECTED_COLUMNS;
static compound** ROOT_CONDITION;

extern int   query_debug;
extern FILE* query_in;

static int query_parse(void);
extern int query_lex();
static void query_error();

int queryparse(const char* query, size_t query_length, growbuf* selected_columns, compound** root_condition)
{
    int fd[2];

    SELECTED_COLUMNS = selected_columns;
    ROOT_CONDITION = root_condition;

    //TODO: need to feed the query string to the bison parser.
    // perhaps via a pipe??? :[

    if (0 != pipe(fd)) {
        perror("error creating pipe for parsing");
        return -1;
    }

    query_in = fdopen(fd[0], "r");
    if (NULL == query_in) {
        perror("error on pipe fdopen for parsing");
        return -1;
    }

    if (query_length != write(fd[1], query, query_length)) {
        perror("error on pipe write for parsing");
        return -1;
    }

    close(fd[1]);

    int retval = query_parse();

    fclose(query_in);

    return retval;
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

void free_compound(compound* c)
{
    if (NULL != c) {
        if (c->simple.rval.is_str && NULL != c->simple.rval.str) {
            free(c->simple.rval.str);
        }
        compound* l = c->left;
        compound* r = c->right;

        free(c);

        free_compound(l);
        free_compound(r);
    }
}

%}

%union {
    long      num;
    char*     str;
    rval      rval;
    compound* compound;
}

%token TOK_SELECT TOK_WHERE TOK_EQ TOK_NEQ TOK_GT TOK_LT TOK_GTE TOK_LTE TOK_AND TOK_OR TOK_NOT TOK_LPAREN TOK_RPAREN TOK_COMMA TOK_DASH TOK_ERROR

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

Columns : Columns TOK_COMMA Columnspec
        | Columnspec
;

Columnspec
    : TOK_COLUMN TOK_DASH TOK_COLUMN {
        for (size_t i = $1; i <= $3; i++) {
            growbuf_append(SELECTED_COLUMNS, &i, sizeof(size_t));
        }
    }
    | TOK_COLUMN {
        size_t col = $1;
        growbuf_append(SELECTED_COLUMNS, &col, sizeof(size_t));
    }
    | { 
        size_t max = SIZE_MAX;
        growbuf_append(SELECTED_COLUMNS, &max, sizeof(size_t));
    }
;

Conditions  : Clause {
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
        | TOK_COLUMN TOK_NEQ Rvalue {
            $$ = new_compound();

            $$->simple.column = $1;
            $$->simple.oper = TOK_NEQ;
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
            $$.col = $1;
            $$.is_col = true;
        }
        | TOK_STRING {
            $$.str = $1;
            $$.is_str = true;
        }
        | TOK_NUMBER {
            $$.num = $1;
            $$.is_num = true;
        }
;

%%

void query_error(const char* s)
{
    fprintf(stderr, "%s\n", s);
}


