/*
 * CSV Selector
 *
 * by William R. Fraser, 10/22/2011
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "growbuf.h"
#include "queryparse.h"
#include "queryparse.tab.h"
#include "util.h"
#include "functions.h"

//#define DEBUG
#define DEBUG if (false)

extern int query_debug;

extern functionspec FUNCTIONS[];

/**
 * Evaluates a val to a constant.
 * Returns a new val with all strings copied.
 */
val value_evaluate(const val* val, growbuf* fields, size_t rownum)
{
    struct _val ret = {};

    if (val->is_num) {
        ret.num = val->num;
        ret.is_num = true;
    }
    else if (val->is_dbl) {
        ret.dbl = val->dbl;
        ret.is_dbl = true;
    }
    else if (val->is_col) {
        ret.str = strdup((char*)((growbuf**)fields->buf)[val->col]->buf);
        ret.is_str = true;
    }
    else if (val->is_special) {
        switch (val->special) {
        case SPECIAL_NUMCOLS:
            ret.num = fields->size / sizeof(void*);
            ret.is_num = true;
            break;
        case SPECIAL_ROWNUM:
            ret.num = rownum;
            ret.is_num = true;
            break;
        }
    }
    else if (val->is_func) {
        struct _val args[MAX_ARGS];
        for (size_t i = 0; i < val->func->num_args; i++) {
            args[i] = value_evaluate(&(val->func->args[i]), fields, rownum);
        }

        switch (val->func->func) {
        case FUNC_SUBSTR:
            {
                if (val->func->num_args == 2) {
                    args[2].num = -1;
                    args[2].is_num = true;
                }

                if (args[2].num < 0) {
                    args[2].num += strlen(args[0].str) + 1;
                }

                size_t start = args[1].num;
                size_t len = args[2].num;
                size_t size = len - start;
                char* result = (char*)malloc(size + 1);
                memcpy(result, args[0].str + start, size);
                result[size] = '\0';

                ret.str = result;
                ret.is_str = true;
            }
            break;

        case FUNC_STRLEN:
            {
                ret.num = strlen(args[0].str);
                ret.is_num = true;
            }
            break;

        default:
            fprintf(stderr, "ERROR: no implementation for function %s\n",
                    FUNCTIONS[val->func->func].name);
        }
    }

    //
    // Evaluation to a constant is now complete.
    // Now, do any explicit type conversion that was specified.
    //

    if (val->conversion_type == TYPE_LONG) {
        if (ret.is_str) {
            char* str = ret.str;
            ret.num = atol(str);
            ret.is_str = false;
            free(str);
        }
        else if (ret.is_dbl) {
            ret.num = (long)ret.dbl;
            ret.is_dbl = false;
        }
        ret.is_num = true;
    }
    else if (val->conversion_type == TYPE_DOUBLE) {
        if (ret.is_str) {
            char* str = ret.str;
            ret.dbl = strtod(str, NULL);
            ret.is_str = false;
            free(str);
        }
        else if (ret.is_num) {
            ret.dbl = (double)ret.num;
            ret.is_num = false;
        }
        ret.is_dbl = true;
    }
    else if (val->conversion_type == TYPE_STRING) {
        if (ret.is_num) {
            //TODO
            ret.str = strdup("<TODO>");
            ret.is_num = false;
        }
        else if (ret.is_dbl) {
            //TODO
            ret.str = strdup("<TODO>");
            ret.is_dbl = false;
        }
        ret.is_str = true;
    }

    return ret;
}

bool query_evaluate(growbuf* fields, size_t rownum, compound* condition)
{
    bool retval = true;

    if (NULL == condition) {
        // return true
        goto cleanup;
    }

    switch (condition->oper) {
    case OPER_SIMPLE:
        {
            val left = value_evaluate(
                                &(condition->simple.left),  fields, rownum);
            val right = value_evaluate(
                                &(condition->simple.right), fields, rownum);

            //
            // Next we do automatic type conversion if needed.
            //
            // conversion | num dbl str
            // -----------+----------------
            //        num | num dbl  *
            //        dbl | dbl dbl dbl
            //        str |  *  dbl str
            //
            // *: If the string has a dot in it, both are converted to dbl.
            //    Otherwise, the string is converted to num.
            //

#define CONVERSION(a, b) \
            if (b.is_dbl) { \
                if (a.is_str) { \
                    char* temp = a.str; \
                    a.dbl = strtod(temp, NULL); \
                    a.is_dbl = true; \
                    a.is_str = false; \
                    free(temp); \
                } \
                else if (a.is_num) { \
                    a.dbl = (double)a.num; \
                    a.is_dbl = true; \
                    a.is_num = false; \
                } \
            } \
            else if (b.is_num && a.is_str) { \
                if (strchr(a.str, '.') != NULL) { \
                    char* temp = a.str; \
                    a.dbl = strtod(temp, NULL); \
                    a.is_dbl = true; \
                    a.is_str = false; \
                    free(temp); \
                    b.dbl = (double)b.num; \
                    b.is_dbl = true; \
                    b.is_num = false; \
                } \
                else { \
                    char* temp = a.str; \
                    a.num = atol(temp); \
                    a.is_num = true; \
                    a.is_str = false; \
                    free(temp); \
                } \
            }
           
            //
            // The macro only modifies one side (except the special case).
            // Need to apply it both ways.
            //
            CONVERSION(left, right);
            CONVERSION(right, left);

#define COMPARE(operator) \
            if (left.is_dbl) { \
                retval = (left.dbl operator right.dbl); \
            } else if (left.is_num) { \
                retval = (left.num operator right.num); \
            } \
            else { \
                retval = (strcmp(left.str, right.str) operator 0); \
            } \

            switch (condition->simple.oper) {
            case TOK_EQ:
                COMPARE(==)
                break;

            case TOK_NEQ:
                COMPARE(!=)
                break;

            case TOK_GT:
                COMPARE(>)
                break;

            case TOK_LT:
                COMPARE(<)
                break;

            case TOK_GTE:
                COMPARE(>=)
                break;

            case TOK_LTE:
                COMPARE(<=)
                break;
            }
        }
        break;

    case OPER_NOT:
        retval = ! query_evaluate(fields, rownum, condition->left);
        break;

    case OPER_AND:
        retval = (query_evaluate(fields, rownum, condition->left) && query_evaluate(fields, rownum, condition->right));
        break;

    case OPER_OR:
        retval = (query_evaluate(fields, rownum, condition->left) || query_evaluate(fields, rownum, condition->right));
        break;
    }

cleanup:
    return retval;
}

