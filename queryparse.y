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
    long          num;
    char*         str;
    size_t        col;
    special_value special;
    rval          rval;
    lval          lval;
    compound*     compound;
    condition     simple;
}

%token TOK_SELECT TOK_WHERE TOK_EQ TOK_NEQ TOK_GT TOK_LT TOK_GTE TOK_LTE TOK_AND TOK_OR TOK_NOT TOK_LPAREN TOK_RPAREN TOK_COMMA TOK_DASH TOK_ERROR

%token <num> TOK_NUMBER
%token <col> TOK_COLUMN
%token <str> TOK_STRING
%token <special> TOK_SPECIAL

%type <lval> Lvalue
%type <rval> Rvalue
%type <compound> Compound
%type <simple> Simple
%type <num> Operator

%error-verbose

%%

Start
    : TOK_SELECT Columns TOK_WHERE Conditions
    | TOK_SELECT Columns
;

Columns
    : Columns TOK_COMMA Columnspec
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

Conditions
    : Compound {
        *ROOT_CONDITION = $1;
    }
;

Simple
    : Lvalue Operator Rvalue {
        $$.lval = $1;
        $$.oper = $2;
        $$.rval = $3;
    }
;

Compound
    : Compound TOK_AND Compound {
        $$ = new_compound();
        $$->left = $1;
        $$->right = $3;
        $$->oper = OPER_AND;
    }
    | Compound TOK_OR Compound {
        $$ = new_compound();
        $$->left = $1;
        $$->right = $3;
        $$->oper = OPER_OR;
    }
    | TOK_NOT Compound {
        $$ = new_compound();
        $$->left = $2;
        $$->oper = OPER_NOT;
    }
    | TOK_LPAREN Compound TOK_RPAREN {
        $$ = $2;
    }
    | Simple {
        $$ = new_compound();
        $$->simple = $1;
        $$->oper = OPER_SIMPLE;
    }
;

Operator
    : TOK_EQ {
        $$ = TOK_EQ;
    }
    | TOK_NEQ {
        $$ = TOK_NEQ;
    }
    | TOK_GT {
        $$ = TOK_GT;
    }
    | TOK_LT {
        $$ = TOK_LT;
    }
    | TOK_GTE {
        $$ = TOK_GTE;
    }
    | TOK_LTE {
        $$ = TOK_LTE;
    }
;

Rvalue
    : TOK_COLUMN {
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

Lvalue
    : TOK_COLUMN {
        $$.col = $1;
        $$.is_col = true;
    }
    | TOK_SPECIAL {
        $$.special = $1;
        $$.is_special = true;
    }
;

%%

void query_error(const char* s)
{
    fprintf(stderr, "%s\n", s);
}


