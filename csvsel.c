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

#define DEBUG

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

bool query_evaluate(growbuf* fields, compound* condition)
{
    bool retval = true;

    switch (condition->oper) {
    case OPER_SIMPLE:
        {
            if (fields->size / sizeof(void*) < condition->simple.column) {
                fprintf(stderr, "invalid column %lu, there are only %zu present.\n",
                        condition->simple.column,
                        fields->size / sizeof(void*));
                return false;
            }

            char* check_field_str = ((growbuf**)(fields->buf))[condition->simple.column]->buf;
            long  check_field_num = 0;
            bool  numeric_comparison = false;

            char* rval_str = NULL;

            if (condition->simple.rval.is_num) {
                if (string_is_num(check_field_str)) {
                    check_field_num = atoi(check_field_str);
                    numeric_comparison = true;
                }
                else {
                    retval = false;
                    break;
                }
            }
            else {
                if (condition->simple.rval.is_col) {
                    if (fields->size / sizeof(void*) < condition->simple.rval.col) {
                        fprintf(stderr, "invalid column %lu, there are only %zu present.\n",
                                condition->simple.rval.col,
                                fields->size / sizeof(void*));
                        return false;
                    }
                    DEBUG fprintf(stderr, "comparing to column %lu\n",
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
    case OPER_NOT:
        retval = ! query_evaluate(fields, condition->left);
        break;

    case OPER_AND:
        retval = (query_evaluate(fields, condition->left) && query_evaluate(fields, condition->right));
        break;

    case OPER_OR:
        retval = (query_evaluate(fields, condition->left) || query_evaluate(fields, condition->right));
        break;
    }

    return retval;
}

int csv_select(FILE* input, const char* query, size_t query_length)
{
    int retval = 0;

    growbuf* selected_columns = NULL;
    compound* root_condition = NULL;

    selected_columns = growbuf_create(1);
    if (NULL == selected_columns) {
        fprintf(stderr, "malloc failed\n");
        retval = 2;
        goto cleanup;
    }

    queryparse(query, query_length, selected_columns, &root_condition);

    fprintf(stderr, "condition:\n");
    print_condition(root_condition, 0);

    size_t line = 0;
    bool in_dquot = false;
    bool prev_was_dquot = false;
    growbuf* fields = growbuf_create(1);
    growbuf* field = growbuf_create(32);
    growbuf_append(fields, field, sizeof(growbuf*));
    while (true) { // iterate over lines
        int char_in = fgetc(input);

        if (EOF == char_in) {
            goto handle_eof;
        }

        char c = (char)char_in;
        DEBUG fprintf(stderr, "(%c)", c);
        switch (c) {
        case '"':
            if (in_dquot) {
                if (prev_was_dquot) {
                    growbuf_append(field, &c, 1);
                    prev_was_dquot = false;
                    DEBUG fprintf(stderr, "no longer in prev_was_dquot");
                }
                else {
                    prev_was_dquot = true;
                    DEBUG fprintf(stderr, "now in prev_was_dquot");
                    // don't append yet, wait for the next char.
                }
            }
            else if (field->size != 0) {
                fprintf(stderr, "csv syntax error: unexpected double-quote in"
                        " line %u field %u\n",
                        line,
                        fields->size / sizeof(void*));
                retval = EX_DATAERR;
                goto cleanup;
            }
            else {
                DEBUG fprintf(stderr, "now in dquot");
                in_dquot = true;
                // don't append.
           }
           break;

       case '\n':
           if (!in_dquot || prev_was_dquot) {
                // we're done with the line
                DEBUG fprintf(stderr, "done with line");
                c = '\0';
                growbuf_append(field, &c, 1);
                    
                if (query_evaluate(fields, root_condition)) {
                    DEBUG printf("line %u matches\n", line);
                }
                else {
                    DEBUG printf("line %u doesn't match.\n", line);
                }

                for (size_t i = 0; i < fields->size / sizeof(void*); i++) {
                    growbuf_free(((growbuf**)(fields->buf))[i]);
                }
                fields->size = 0;
                field = growbuf_create(32);
                growbuf_append(fields, field, sizeof(void*));
                in_dquot = false;
                prev_was_dquot = false;
                line++;
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
                DEBUG fprintf(stderr, "done with field");
                field = growbuf_create(32);
                growbuf_append(fields, field, sizeof(void*));
                in_dquot = false;
                prev_was_dquot = false;
            }
            else {
                growbuf_append(field, &c, 1);
            }
            break;

        default:
            if (in_dquot) {
                if (prev_was_dquot) {
                    in_dquot = false;
                    DEBUG fprintf(stderr, "not in dquot");
                    // the field better end after this, or else badness.
                }
            }
            else {
                growbuf_append(field, &c, 1);
            }
            break;

        } // switch

    } // while (true)

handle_eof:
    
    if (query_evaluate(fields, root_condition)) {
        DEBUG printf("line %u matches\n", line);
    }
    else {
        DEBUG printf("line %u doesn't match\n", line);
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

    return retval;
}
