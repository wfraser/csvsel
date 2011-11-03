/*
 * CSV Selector
 *
 * by William R. Fraser, 10/22/2011
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sysexits.h>
#include <string.h>
#include <errno.h>

#include "growbuf.h"
#include "csvformat.h"
#include "queryeval.h"
#include "util.h"

#define DEBUG if (false)
//#define DEBUG

int query_debug = 0;

typedef struct {
    compound* root_condition;
    growbuf*  selected_columns;
} row_evaluator_args;

void eval_and_print(growbuf* fields, size_t rownum, row_evaluator_args* args)
{
    if (query_evaluate(fields, rownum, args->root_condition)) {
        print_selected_columns(fields, args->selected_columns);
    }
}

int csv_select(FILE* input, const char* query, size_t query_len)
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

    if (0 != queryparse(query, query_len, selected_columns, &root_condition))
    {
        retval = 1;
        goto cleanup;
    }

    if (query_debug)
    {
        fprintf(stderr, "condition:\n");
        print_condition(root_condition, 0);
    }
    
    row_evaluator_args args = { root_condition, selected_columns };
    
    retval = read_csv(input, (row_evaluator)&eval_and_print, (void*)&args);
    
cleanup:
    if (NULL != selected_columns) {
        growbuf_free(selected_columns);
    }
    
    if (NULL != root_condition) {
        free_compound(root_condition);
    }

    return retval;
}

int main(int argc, char** argv)
{
    int    retval          = EX_OK;
    FILE*  input           = stdin;
    size_t query_arg_start = 1;

    growbuf* query = growbuf_create(32);
    if (NULL == query) {
        fprintf(stderr, "malloc failed\n");
        retval = EX_OSERR;
        goto cleanup;
    }

    if (argc == 1) {
        fprintf(stderr, "usage: %s [-f inputfile] <query string>\n", argv[0]);
        retval = EX_USAGE;
        goto cleanup;
    }

    for (size_t i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0
                || strcmp(argv[i], "--debug") == 0) {
            query_debug = 1;
            query_arg_start = i + 1;
        }
        else if (strcmp(argv[i], "-f") == 0
                || strcmp(argv[i], "--file") == 0) {

            if (argc < 3) {
                fprintf(stderr, "%s flag needs an argument\n", argv[i]);
                retval = EX_USAGE;
                goto cleanup;
            }

            if (0 != access(argv[i + 1], R_OK)) {
                perror("unable to access input file");
                retval = EX_NOINPUT;
                goto cleanup;
            }

            if (input != NULL) {
                fclose(input);
            }

            input = fopen(argv[i + 1], "r");
            if (NULL == input) {
                perror("opening input failed");
                retval = EX_NOINPUT;
                goto cleanup;
            }

            query_arg_start = i + 2;
        }
        else {
            break;
        }
    }

    //
    // accumulate all remaining command line arguments into one
    // space-separated string
    //

    const char* space = " ";
    for (size_t i = query_arg_start; i < argc; i++) {
        size_t len = strlen(argv[i]);
        growbuf_append(query, argv[i], len);
        growbuf_append(query, space, 1);
    }
    ((char*)query->buf)[ query->size - 1 ] = '\0';

    DEBUG printf("%s\n", (char*)query->buf);

    switch (csv_select(input, query->buf, query->size)) {
        case 0:
            retval = EX_OK;
            break;
        
        case 1:
            retval = EX_USAGE;
            break;

        default:
            retval = EX_OSERR;
            break;
    }

cleanup:
    if (NULL != query) {
        growbuf_free(query);
    }

    if (NULL != input) {
        fclose(input);
    }

    return retval;
}
