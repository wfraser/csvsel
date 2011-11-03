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

#include "csvsel.h"

//#define DEBUG
#define DEBUG if (false)

extern int query_debug;

void print_selected_columns(growbuf* fields, growbuf* selected_columns)
{
    size_t first_col = ((size_t*)selected_columns->buf)[0];

    size_t num_selected_columns = selected_columns->size / sizeof(size_t);
    size_t num_columns = fields->size / sizeof(void*);
    size_t limit = (first_col == -1) ? num_columns : num_selected_columns;
    
    for (size_t i = 0; i < limit; i++)
    {
        size_t col = (first_col == -1) ? i : ((size_t*)selected_columns->buf)[i];
        
        char* colstr = (char*)((growbuf**)fields->buf)[col]->buf;

        if (NULL != strchr(colstr, ' ')) {
            printf("\"");
            for (size_t j = 0; j < strlen(colstr); j++) {
                if (colstr[j] == '"') {
                    printf("\"\"");
                }
                else {
                    printf("%c", colstr[j]);
                }
            }
            printf("\"");
        }
        else {
            printf("%s", colstr);
        }

        if (i != limit - 1) {
            printf(",");
        }
        else {
            printf("\n");
        }
    }
}

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
        switch (val->func->func) {
        case FUNC_SUBSTR:
            {
                struct _val arg1 = value_evaluate(
                                        &(val->func->arg1), fields, rownum);
                struct _val arg2 = value_evaluate(
                                        &(val->func->arg2), fields, rownum);

                struct _val arg3 = {};
                if (val->func->num_args > 2) {
                    arg3 = value_evaluate(
                                        &(val->func->arg3), fields, rownum);
                }
                else {
                    arg3.num = -1;
                    arg3.is_num = true;
                }

                if (arg3.num < 0) {
                    arg3.num += strlen(arg1.str) + 1;
                }

                size_t start = arg2.num;
                size_t len = arg3.num;
                size_t size = len - start;
                char* result = (char*)malloc(size + 1);
                memcpy(result, arg1.str + start, size);
                result[size] = '\0';

                ret.str = result;
                ret.is_str = true;
            }
            break;
        default:
            fprintf(stderr, "ERROR: unknown function to evaluate!!\n");
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

bool query_evaluate(growbuf* fields, compound* condition, size_t rownum)
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
        retval = ! query_evaluate(fields, condition->left, rownum);
        break;

    case OPER_AND:
        retval = (query_evaluate(fields, condition->left, rownum) && query_evaluate(fields, condition->right, rownum));
        break;

    case OPER_OR:
        retval = (query_evaluate(fields, condition->left, rownum) || query_evaluate(fields, condition->right, rownum));
        break;
    }

cleanup:
    return retval;
}

int csv_select(FILE* input, const char* query, size_t query_length)
{
    int retval = 0;
    growbuf* fields = NULL;
    growbuf* field  = NULL;

    growbuf* selected_columns = NULL;
    compound* root_condition = NULL;

    selected_columns = growbuf_create(1);
    if (NULL == selected_columns) {
        fprintf(stderr, "malloc failed\n");
        retval = 2;
        goto cleanup;
    }

    if (0 != queryparse(query, query_length, selected_columns, &root_condition))
    {
        retval = 3;
        goto cleanup;
    }

    if (query_debug)
    {
        fprintf(stderr, "condition:\n");
        print_condition(root_condition, 0);
    }

    size_t rownum = 0;
    bool in_dquot = false;
    bool prev_was_dquot = false;

    fields = growbuf_create(1);
    field = growbuf_create(32);
    growbuf_append(fields, &field, sizeof(growbuf*));
    while (true) { // iterate over lines
        int char_in = fgetc(input);

        if (EOF == char_in) {
            goto handle_eof;
        }

        char c = (char)char_in;
        switch (c) {
        case '"':
            if (in_dquot) {
                if (prev_was_dquot) {
                    growbuf_append(field, &c, 1);
                    prev_was_dquot = false;
                }
                else {
                    prev_was_dquot = true;
                    // don't append yet, wait for the next char.
                }
            }
            else if (field->size != 0) {
                fprintf(stderr, "csv syntax error: unexpected double-quote in"
                        " line %u field %u\n",
                        rownum,
                        fields->size / sizeof(void*));
                retval = EX_DATAERR;
                goto cleanup;
            }
            else {
                in_dquot = true;
                // don't append.
           }
           break;

       case '\n':
           if (!in_dquot || prev_was_dquot) {
                // we're done with the line
                c = '\0';
                growbuf_append(field, &c, 1);

                DEBUG for (size_t i = 0; i < fields->size / sizeof(void*); i++)
                {
                    fprintf(stderr, "field %zu: ", i);
                    fprintf(stderr, "\"%s\"\n",
                        (char*)(((growbuf**)fields->buf)[i]->buf)
                    );
                }

                if (fields->size / sizeof(void*) > 0
                        && (fields->size / sizeof(void*) > 1
                            || ((growbuf**)fields->buf)[0]->size > 0))
                {
                    if (query_evaluate(fields, root_condition, rownum)) {
                        print_selected_columns(fields, selected_columns);
                    }
                }

                for (size_t i = 0; i < fields->size / sizeof(void*); i++) {
                    growbuf_free(((growbuf**)(fields->buf))[i]);
                }
                fields->size = 0;
                field = growbuf_create(32);
                growbuf_append(fields, &field, sizeof(void*));
                in_dquot = false;
                prev_was_dquot = false;
                rownum++;
                break;
            }
            else {
                // embedded newline
                growbuf_append(field, &c, 1);
            }
            break;

        case ',':
            if (!in_dquot || prev_was_dquot) {
                // we're done with the field
                c = '\0';
                growbuf_append(field, &c, 1);

                field = growbuf_create(32);
                growbuf_append(fields, &field, sizeof(void*));
                in_dquot = false;
                prev_was_dquot = false;
            }
            else {
                growbuf_append(field, &c, 1);
            }
            break;

        default:
            if (in_dquot && prev_was_dquot) {
                in_dquot = false;
                // the field better end after this, or else badness.
            }
            else {
                growbuf_append(field, &c, 1);
            }
            break;

        } // switch

    } // while (true)

handle_eof:

    if (fields->size / sizeof(void*) > 0
            && ((fields->size / sizeof(void*) > 1 
                 || ((growbuf**)fields->buf)[0]->size > 0)))
    {
        if (query_evaluate(fields, root_condition, rownum)) {
            print_selected_columns(fields, selected_columns);
        }
    }

cleanup:
    if (NULL != selected_columns) {
        growbuf_free(selected_columns);
    }

    if (NULL != fields) {
        for (size_t i = 0; i < fields->size / sizeof(void*); i++) {
            growbuf_free(((growbuf**)(fields->buf))[i]);
        }
        growbuf_free(fields);
    }

    if (NULL != root_condition) {
        free_compound(root_condition);
    }

    return retval;
}
