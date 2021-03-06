#ifndef CSVSEL_H
#define CSVSEL_H

#include <stdio.h>
#include "growbuf.h"
#include "queryeval.h"

typedef struct {
    compound* root_condition;
    growbuf*  selectors;
    FILE*     output;
} row_evaluator_args;

int csv_select(FILE* input, FILE* output, const char* query, size_t query_len);

#endif //CSVSEL_H

