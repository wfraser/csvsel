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

bool string_is_num(const char* string)
{
    size_t len = strlen(string);
    for (size_t i = 0; i < len; i++) {
        if (string[i] > '9' || string[i] < '0') {
            return false;
        }
    }

    return true;
}

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
            char*  check_field_str = NULL;
            long   check_field_num = 0;
            bool   numeric_comparison = false;

            if (condition->simple.lval.is_col) {

                if (fields->size / sizeof(void*) < condition->simple.lval.col) {
                    fprintf(stderr, "invalid column %zu, there are only %zu present.\n",
                            condition->simple.lval.col,
                            fields->size / sizeof(void*));
                    retval = false;
                    goto cleanup;
                }

                check_field_str = ((growbuf**)(fields->buf))[condition->simple.lval.col]->buf;

            }
            if (condition->simple.lval.is_special) {
                switch (condition->simple.lval.special) {
                case SPECIAL_NUMCOLS:
                    check_field_num = fields->size / sizeof(void*);
                    numeric_comparison = true;
                    break;
                case SPECIAL_ROWNUM:
                    check_field_num = rownum;
                    numeric_comparison = true;
                    break;
                default:
                    fprintf(stderr, "unknown special value\n");
                    retval = false;
                    goto cleanup;
                }
            }

            char* rval_str = NULL;

            if (condition->simple.rval.is_num) {
                if (!numeric_comparison) {
                    if (string_is_num(check_field_str)) {
                        check_field_num = atoi(check_field_str);
                        numeric_comparison = true;
                    }
                    else {
                        retval = false;
                        break;
                    }
                }
            }
            else {
                if (condition->simple.rval.is_col) {
                    if (fields->size / sizeof(void*) < condition->simple.rval.col) {
                        fprintf(stderr, "invalid column %zu, there are only %zu present.\n",
                                condition->simple.rval.col,
                                fields->size / sizeof(void*));
                        retval = false;
                        goto cleanup;
                    }
                    DEBUG fprintf(stderr, "comparing to column %zu\n",
                            condition->simple.rval.col);

                    rval_str = ((growbuf**)(fields->buf))[condition->simple.rval.col]->buf;
                    DEBUG fprintf(stderr, "rval(%s)\n", rval_str);
                }
                else {
                    rval_str = condition->simple.rval.str;
                }
            }

#define COMPARE(operator) \
    if (numeric_comparison) { \
        retval = (check_field_num operator condition->simple.rval.num); \
    } \
    else { \
        retval = (strcmp(check_field_str, rval_str) operator 0); \
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

                if (query_evaluate(fields, root_condition, rownum)) {
                    print_selected_columns(fields, selected_columns);
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
            && (fields->size / sizeof(void*) == 1 
                ? ((growbuf**)fields->buf)[0]->size > 0
                : false))
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
