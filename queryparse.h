#ifndef QUERY_H
#define QUERY_H

#include <stdlib.h>
#include <stdbool.h>
#include <sysexits.h>
#include "growbuf.h"

typedef enum
{
    SPECIAL_NUMCOLS, SPECIAL_ROWNUM
}
special_value;

typedef struct
{
    union {
        long          num;
        double        dbl;
        char*         str;
        size_t        col;
        special_value special;
    };
    bool is_num;
    bool is_dbl;
    bool is_str;
    bool is_col;
    bool is_special;
} val;

typedef struct
{
    int oper;
    val left;
    val right;
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

void free_compound(compound* c);

#endif // QUERY_H
