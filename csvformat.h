#ifndef CSVSEL_H
#define CSVSEL_H

#include <stdio.h>

typedef void (*row_evaluator)(growbuf* fields, size_t rownum, void* context);

void print_csv_field(const char* field, FILE* output);
int read_csv(FILE* input, row_evaluator row_evaluator, void* context);

#endif // CSVSEL_H
