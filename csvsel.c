#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "growbuf.h"
#include "queryparse.h"
#include "queryparse.tab.h"

#include "csvsel.h"

bool query_evaluate(growbuf* fields, compound* condition)
{
    bool retval = true;

    switch (condition->oper) {
    case OPER_SIMPLE:
        {
            //char* check_field = ((growbuf**)(fields->buf))[condition->simple.column]->buf;

            // have to check if the checked column is a numeric string and parse it accordingly;
            // this affects the behavior of the comparisons.

            // TODO: what if it's not a numeric string, but is compared against a number rval?

            switch (condition->simple.oper) {
            case TOK_EQ:
            case TOK_GT:
            case TOK_LT:
            case TOK_GTE:
            case TOK_LTE:
                //TODO
            break;
            }
        }
    case OPER_NOT:
    case OPER_AND:
    case OPER_OR:
        //TODO
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

    //TODO

cleanup:
    if (NULL != selected_columns) {
        growbuf_free(selected_columns);
    }

    return retval;
}
