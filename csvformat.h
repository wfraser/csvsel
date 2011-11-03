#ifndef CSVSEL_H
#define CSVSEL_H

#include <stdio.h>

typedef void (*row_evaluator)(growbuf* fields, size_t rownum, void* context);

void print_selected_columns(growbuf* fields, growbuf* selected_columns);

int read_csv(FILE* input, row_evaluator row_evaluator, void* context);

#endif // CSVSEL_H
