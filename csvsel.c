#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "growbuf.h"
#include "query.h"

#include "csvsel.h"

int csv_select(FILE* input, const char* query, size_t query_length)
{
    int retval = 0;

    growbuf* selected_columns = NULL;
    growbuf* conditions = NULL;

    selected_columns = growbuf_create(1);
    if (NULL == selected_columns) {
        fprintf(stderr, "malloc failed\n");
        retval = 2;
        goto cleanup;
    }

    conditions = growbuf_create(1);
    if (NULL == conditions) {
        fprintf(stderr, "malloc failed\n");
        retval = 2;
        goto cleanup;
    }

    queryparse(query, query_length, selected_columns, conditions);

    //TODO

cleanup:
    if (NULL != selected_columns) {
        growbuf_free(selected_columns);
    }

    if (NULL != conditions) {
        growbuf_free(conditions);
    }

    return retval;
}
