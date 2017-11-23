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

static void print_field(val v, uint64_t byte_offset, size_t field_num, size_t total_fields, void* context)
{
    FILE* output = (FILE*)context;
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

static void evaluate_selector(
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

static void eval_and_print(growbuf* fields, size_t rownum, uint64_t byte_offset, void* context)
{
    row_evaluator_args* args = (row_evaluator_args*)context;
    FILE* output = args->output;
    compound* root_condition = args->root_condition;
    growbuf* selectors = args->selectors;
    size_t num_selectors = selectors->size / sizeof(void*);

    if (query_evaluate(fields, rownum, root_condition)) {
        for (size_t sel_num = 0; sel_num < num_selectors; sel_num++) {
            selector* c = ((selector**)(selectors->buf))[sel_num];
            evaluate_selector(c, fields, rownum, byte_offset, sel_num, num_selectors,
                    &print_field, output);
        }
    }
}

typedef struct {
    int row_number;
    uint64_t byte_offset;
    val value;
} row_sort_data;

typedef struct {
    compound* root_condition;
    selector* order_selector;
    growbuf* sort_data;
} sort_args;

typedef struct {
    growbuf* sort_data;
    int row_number;
} row_sort_args;

static void populate_sort_data_field(
        val value,
        uint64_t byte_offset,
        size_t field_num,
        size_t total_fields,
        void* context)
{
    row_sort_args* args = (row_sort_args*)context;
    row_sort_data d = {
        args->row_number,
        byte_offset,
        value,
    };
    if (value.is_str) {
        d.value.str = strdup(value.str);
    }
    growbuf_append(args->sort_data, &d, sizeof(d));
}

static void populate_sort_data(growbuf* fields, size_t rownum, uint64_t byte_offset, void* context)
{
    sort_args* args = (sort_args*)context;
    compound* root_condition = args->root_condition;
    selector* order_selector = args->order_selector;

    if (query_evaluate(fields, rownum, root_condition)) {
        row_sort_args row_args = {
            args->sort_data,
            rownum,
        };
        evaluate_selector(order_selector, fields, rownum, byte_offset, 0, 1,
                &populate_sort_data_field, &row_args);
    }
}

static int row_comparator(const void* avoid, const void* bvoid)
{
    // The values are expected to be of the same type.
    row_sort_data* a = (row_sort_data*)avoid;
    row_sort_data* b = (row_sort_data*)bvoid;
    if (a->value.is_str) {
        return strcmp(a->value.str, b->value.str);
    } else if (a->value.is_num) {
        return a->value.num - b->value.num;
    } else if (a->value.is_dbl) {
        return a->value.dbl - b->value.dbl;
    } else {
        fprintf(stderr, "error: invalid value type in row comparator: ");

        if (a->value.is_col) {
            fprintf(stderr, "column\n");
        } else if (a->value.is_special) {
            fprintf(stderr, "special\n");
        } else if (a->value.is_func) {
            fprintf(stderr, "func\n");
        } else {
            fprintf(stderr, "none\n");
        }
        return 0;
    }
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

    row_evaluator_args print_args = { root_condition, selectors, output };

    if (order != NULL) {
        // Read the file, accumulating the sort fields and row byte offsets.

        selector s = {0};
        s.type = SELECTOR_VALUE;
        s.value = order->value;

        growbuf* sort_data = growbuf_create(0);

        sort_args sort_args = {
            root_condition,
            &s,
            sort_data,
        };

        if (0 != read_csv(input, &populate_sort_data, &sort_args)) {
            retval = EX_DATAERR;
            goto cleanup;
        }

        size_t num_rows = sort_data->size / sizeof(row_sort_data);
        qsort(sort_data->buf, num_rows, sizeof(row_sort_data), &row_comparator);

        bool reverse = (order->direction == ORDER_DESCENDING);
        for (size_t i = reverse ? num_rows - 1 : i;
                reverse ? i < num_rows : i < num_rows;
                reverse ? i-- : i++)
        {
            row_sort_data* row = &(((row_sort_data*)sort_data->buf)[i]);

            if (-1 == fseek(input, row->byte_offset, SEEK_SET)) {
                perror("error seeking input file");
                retval = EX_DATAERR;
                goto cleanup;
            }

            if (0 != read_csv_row(input, row->row_number, &eval_and_print, &print_args)) {
                retval = EX_DATAERR;
                goto cleanup;
            }

            if (row->value.is_str) {
                free(row->value.str);
                row->value.str = NULL;
            }
        }

    } else {
        // No sort; just read the file and print in one pass.
        if (0 != read_csv(input, &eval_and_print, &print_args)) {
            retval = EX_DATAERR;
        }
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

