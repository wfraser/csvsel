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

void value_to_const(val* val, growbuf* fields, size_t rownum)
{
    if (val->is_col) {
        val->str = (char*)((growbuf**)fields->buf)[val->col]->buf;
    
        if (val->is_num) {
            val->num = atol(val->str);
        }
        else if (val->is_dbl) {
            val->dbl = strtod(val->str, NULL);
        }
        else {
            val->is_str = true;
        }

        val->is_col = false;
    }
    else if (val->is_special) {
        switch (val->special) {
        case SPECIAL_NUMCOLS:
            val->num = fields->size / sizeof(void*);
            val->is_num = true;
            val->is_special = false;
            break;
        case SPECIAL_ROWNUM:
            val->num = rownum;
            val->is_num = true;
            val->is_special = false;
            break;
        }
    }
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
            val left = condition->simple.left;
            val right = condition->simple.right;

            value_to_const(&left,  fields, rownum);
            value_to_const(&right, fields, rownum);

#define CONVERSION(a, b) \
    if (b.is_dbl) { \
        if (a.is_str) { \
            a.dbl = strtod(a.str, NULL); \
            a.is_dbl = true; \
            a.is_num = false; \
        } \
        else if (a.is_num) { \
            a.dbl = (double)a.num; \
            a.is_dbl = true; \
            a.is_num = false; \
        } \
    } \
    else if (b.is_num && a.is_str) { \
        a.num = atol(a.str); \
        a.is_num = true; \
        a.is_str = false; \
    }
            
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
