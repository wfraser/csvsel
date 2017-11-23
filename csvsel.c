/*
 * CSV Selector
 *
 * by William R. Fraser, 10/22/2011
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sysexits.h>
#include <string.h>
#include <errno.h>

#include "growbuf.h"
#include "csvformat.h"
#include "queryeval.h"
#include "util.h"
#include "csvsel.h"

#define DEBUG if (false)
//#define DEBUG

int query_debug = 0;

typedef void (*field_evaluator)(
        val value,
        uint64_t byte_offset,
        size_t field_num,
        size_t total_fields,
        void* context);

static void print_field(val v, uint64_t byte_offset, size_t field_num, size_t total_fields, FILE* output)
{
    if (v.is_num) {
        fprintf(output, "%ld", v.num);
    } else if (v.is_dbl) {
        fprintf(output, "%lf", v.dbl);
    } else if (v.is_str) {
        print_csv_field(v.str, output);
    } else {
        fprintf(stderr, "Error: invalid value type!");
    }

    if (field_num + 1 != total_fields) {
        fprintf(output, ",");
    } else {
        fprintf(output, "\n");
    }
}

void evaluate_selector(
        selector* c,
        growbuf* fields,
        size_t rownum,
        size_t byte_offset,
        size_t selector_num,
        size_t num_selectors,
        field_evaluator eval,
        void* context)
{
    switch (c->type) {
    case SELECTOR_COLUMN:
        if (c->column == SIZE_MAX) {
            size_t num_fields = fields->size / sizeof(void*);
            for (size_t j = 0; j < num_fields; j++) {
                growbuf* field = ((growbuf**)(fields->buf))[j];
                val v = {0};
                v.str = (char*)field->buf;
                v.is_str = true;
                eval(v, byte_offset, selector_num + j, num_selectors + num_fields, context);
            }
        }
        else if (c->column >= (fields->size / sizeof(void*))) {
            //
            // An out-of-bounds column is defined as empty string.
            //

            val v = {0};
            v.str = "";
            v.is_str = true;
            eval(v, byte_offset, selector_num, num_selectors, context);
        }
        else {
            growbuf* field = ((growbuf**)(fields->buf))[c->column];
            val v = {0};
            v.str = (char*)field->buf;
            v.is_str = true;
            eval(v, byte_offset, selector_num, num_selectors, context);
        }
        break;

    case SELECTOR_VALUE:
        {
            val v = value_evaluate(&(c->value), fields, rownum);
            eval(v, byte_offset, selector_num, num_selectors, context);
            val_free(&v);
        }
        break;
    }
}

void eval_and_print(growbuf* fields, size_t rownum, size_t byte_offset, row_evaluator_args* args)
{
    FILE* output = args->output;
    compound* root_condition = args->root_condition;
    growbuf* selectors = args->selectors;
    size_t num_selectors = selectors->size / sizeof(void*);

    if (query_evaluate(fields, rownum, root_condition)) {
        for (size_t sel_num = 0; sel_num < num_selectors; sel_num++) {
            selector* c = ((selector**)(selectors->buf))[sel_num];
            evaluate_selector(c, fields, rownum, byte_offset, sel_num, num_selectors,
                    (field_evaluator)&print_field, output);
        }
    }
}

void populate_sort_data_field(
        val value,
        uint64_t byte_offset,
        size_t field_num,
        size_t total_fields,
        void* context)
{
    //TODO
}

void populate_sort_data(growbuf* fields, size_t rownum, size_t byte_offset, order* order)
{
    fprintf(stderr, "sorting is not implemented yet.");
}

int csv_select(FILE* input, FILE* output, const char* query, size_t query_len)
{
    int retval = 0;

    growbuf* selectors = NULL;
    compound* root_condition = NULL;
    order* order = NULL;

    selectors = growbuf_create(1);
    if (NULL == selectors) {
        fprintf(stderr, "malloc failed\n");
        retval = 2;
        goto cleanup;
    }

    if (0 != queryparse(query, query_len, selectors, &root_condition, &order))
    {
        retval = 1;
        goto cleanup;
    }

    if (query_debug)
    {
        fprintf(stderr, "condition:\n");
        print_condition(root_condition, 0);
    }

    if (order != NULL) {
        fprintf(stderr, "here's where we would read the file and sort.\n");
        if (0 != read_csv(input, (row_evaluator)&populate_sort_data, (void*)order)) {
            retval = EX_DATAERR;
            goto cleanup;
        }
    }
    
    row_evaluator_args args = { root_condition, selectors, output };
    
    if (0 != read_csv(input, (row_evaluator)&eval_and_print, (void*)&args)) {
        retval = EX_DATAERR;
    }

cleanup:
    if (NULL != selectors) {
        for (size_t i = 0; i < selectors->size / sizeof(void*); i++) {
            selector* s = ((selector**)selectors->buf)[i];
            if (NULL != s) {
                selector_free(s);
                free(s);
            }
        }
        growbuf_free(selectors);
    }
    
    if (NULL != root_condition) {
        free_compound(root_condition);
    }

    return retval;
}

