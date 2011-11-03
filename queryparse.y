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
#include "util.h"

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
        if (c->simple.right.is_str && NULL != c->simple.right.str) {
            free(c->simple.right.str);
        }
        if (c->simple.left.is_str && NULL != c->simple.left.str) {
            free(c->simple.left.str);
        }
        compound* l = c->left;
        compound* r = c->right;

        free(c);

        free_compound(l);
        free_compound(r);
    }
}

void value_clear(val* v)
{
    memset(v, 0, sizeof(val));
}

bool check_function(func* f)
{
    switch (f->func) {
    case FUNC_SUBSTR:
        if (f->num_args != 2 && f->num_args != 3) {
            fprintf(stderr, "Error: substr() needs 2 or 3 arguments.\n");
            return false;
        }
        if (f->arg1.conversion_type != TYPE_STRING) {
            fprintf(stderr, "Error: arg 1 of substr() is wrong type.\n");
            return false;
        }
        if (f->arg2.conversion_type != TYPE_LONG) {
            fprintf(stderr, "Error: arg 2 of substr() is wrong type.\n");
            return false;
        }
        if (f->arg3.conversion_type != TYPE_LONG) {
            fprintf(stderr, "Error: arg 3 of substr() is wrong type.\n");
            return false;
        }
        break;
    }

    return true;
}

%}

%union {
    long          num;
    double        dbl;
    char*         str;
    size_t        col;
    special_value special;
    val           val;
    func          func;
    type          type;
    compound*     compound;
    condition     simple;
}

%token TOK_SELECT TOK_WHERE TOK_EQ TOK_NEQ TOK_GT TOK_LT TOK_GTE TOK_LTE TOK_AND TOK_OR TOK_NOT TOK_LPAREN TOK_RPAREN TOK_COMMA TOK_DASH TOK_CONV_NUM TOK_CONV_DBL TOK_CONV_STR TOK_ERROR

%token <num> TOK_INTEGER
%token <dbl> TOK_FLOAT
%token <col> TOK_COLUMN
%token <str> TOK_STRING
%token <str> TOK_IDENTIFIER
%token <special> TOK_SPECIAL

%type <val> Value_Base
%type <val> Value
%type <compound> Compound
%type <func> Function
%type <func> Function_Identifier
%type <simple> Simple
%type <num> Operator
%type <type> Conversion

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
    : Value Operator Value {
        $$.left = $1;
        $$.oper = $2;
        $$.right = $3;
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

Function_Identifier
    : TOK_IDENTIFIER {
        memset(&$$, 0, sizeof(func));

        if (strcmp($1, "substr") == 0) {
            $$.func = FUNC_SUBSTR;
        }
        else {
            fprintf(stderr, "Parse error: unknown function \"%s\"\n", $1);
            YYERROR;
        }

        $$.func_str = $1;
    }
;

Function
    : Function_Identifier TOK_LPAREN TOK_RPAREN {
        $$ = $1;
        $$.num_args = 0;

        if (!check_function(&$$)) {
            YYERROR;
        }
        free($$.func_str);
    }
    | Function_Identifier TOK_LPAREN Value TOK_RPAREN {
        $$ = $1;
        $$.arg1 = $3;
        $$.num_args = 1;

        if (!check_function(&$$)) {
            YYERROR;
        }
        free($$.func_str);
    }
    | Function_Identifier TOK_LPAREN Value TOK_COMMA Value TOK_RPAREN {
        $$ = $1;
        $$.arg1 = $3;
        $$.arg2 = $5;
        $$.num_args = 2;

        if (!check_function(&$$)) {
            YYERROR;
        }
        free($$.func_str);
    }
    | Function_Identifier TOK_LPAREN Value TOK_COMMA Value TOK_COMMA Value TOK_RPAREN {
        $$ = $1;
        $$.arg1 = $3;
        $$.arg2 = $5;
        $$.arg3 = $7;
        $$.num_args = 3;

        if (!check_function(&$$)) {
            YYERROR;
        }
        free($$.func_str);
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

Conversion
    : TOK_CONV_NUM {
        $$ = TYPE_LONG;
    }
    | TOK_CONV_DBL {
        $$ = TYPE_DOUBLE;
    }
    | TOK_CONV_STR {
        $$ = TYPE_STRING;
    }
;

Value
    : Value_Base Conversion {
        $$ = $1;
        $$.conversion_type = $2;
    }
    | Value_Base {
        $$ = $1;
    }
;

Value_Base
    : TOK_COLUMN {
        value_clear(&$$);
        $$.col = $1;
        $$.is_col = true;
        $$.conversion_type = TYPE_STRING;
    }
    | TOK_STRING {
        value_clear(&$$);
        $$.str = $1;
        $$.is_str = true;
        $$.conversion_type = TYPE_STRING;
    }
    | TOK_INTEGER {  
        value_clear(&$$);
        $$.num = $1;
        $$.is_num = true;
        $$.conversion_type = TYPE_LONG;
    }
    | TOK_FLOAT {
        value_clear(&$$);
        $$.dbl = $1;
        $$.is_dbl = true;
        $$.conversion_type = TYPE_DOUBLE;
    }
    | TOK_SPECIAL {
        value_clear(&$$);
        $$.special = $1;
        $$.is_special = true;
        $$.conversion_type = TYPE_LONG;
    }
    | Function {
        value_clear(&$$);
        $$.func = (func*)malloc(sizeof(func));
        memcpy($$.func, &$1, sizeof(func));
        $$.is_func = true;

        switch ($$.func->func) {
        case FUNC_SUBSTR:
            $$.conversion_type = TYPE_STRING;
            break;
        }
    }
;

%%

void query_error(const char* s)
{
    fprintf(stderr, "%s\n", s);
}


