#ifndef QUERY_H
#define QUERY_H

#include <stdlib.h>
#include <stdbool.h>
#include <sysexits.h>
#include "growbuf.h"

typedef struct _rval
{
    union {
        long  num;
        char* str;
        long  col;
    };
    bool is_num;
    bool is_str;
    bool is_col;
} rval;

typedef struct
{
    int    oper;
    long   column;
    rval   rval;
} condition;

typedef struct _compound
{
    struct _compound* left;
    struct _compound* right;
    condition simple;
    enum {
        OPER_AND, OPER_OR, OPER_NOT, OPER_SIMPLE
    } oper;
} compound;

int queryparse(const char* query, size_t query_length, growbuf* selected_columns, compound** root_condition);

#endif // QUERY_H
