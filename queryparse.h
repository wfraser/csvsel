#ifndef QUERY_H
#define QUERY_H

#include <stdlib.h>
#include <stdbool.h>
#include <sysexits.h>
#include "growbuf.h"

typedef enum {
    SPECIAL_NUMCOLS, SPECIAL_ROWNUM
} special_value;

typedef enum {
    FUNC_SUBSTR,
    FUNC_STRLEN,
    FUNC_MAX,
    FUNC_MIN,
    FUNC_ABS,
    FUNC_LOWER,
    FUNC_UPPER,
    FUNC_TRIM,
    MAX_FUNC
} function;

typedef enum {
    TYPE_LONG, TYPE_DOUBLE, TYPE_STRING
} type;

struct _func;

typedef struct _val {
    union {
        long          num;
        double        dbl;
        char*         str;
        size_t        col;
        special_value special;
        struct _func* func;
    };
    bool is_num;
    bool is_dbl;
    bool is_str;
    bool is_col;
    bool is_special;
    bool is_func;
    type conversion_type;
} val;

typedef struct _func {
    val args[3];
    size_t num_args;
    function func;
    char* func_str;
} func;

typedef struct {
    int oper;
    val left;
    val right;
} condition;

typedef struct _compound {
    struct _compound* left;
    struct _compound* right;
    condition simple;
    enum {
        OPER_AND, OPER_OR, OPER_NOT, OPER_SIMPLE
    } oper;
} compound;

typedef struct _selector {
    union {
        size_t column;
        val value;
    };
    enum {
        SELECTOR_COLUMN, SELECTOR_VALUE
    } type;
} selector;

typedef struct _order {
    enum { ORDER_ASCENDING, ORDER_DESCENDING} direction;
    val* value;
} order;

int queryparse(
        const char* query,
        size_t query_length,
        growbuf* selected_columns,
        compound** root_condition,
        order** order);

void free_compound(compound* c);

void free_selectors(growbuf* g);

#endif // QUERY_H
