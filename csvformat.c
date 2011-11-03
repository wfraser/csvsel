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
#include "queryeval.h"

#include "csvformat.h"

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
