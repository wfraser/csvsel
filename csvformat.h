#ifndef CSVFORMAT_H
#define CSVFORMAT_H

#include <stdio.h>
#include <stdint.h>

typedef void (*row_evaluator)(growbuf* fields, size_t rownum, uint64_t byte_offset,  void* context);

void print_csv_field(const char* field, FILE* output);
int read_csv(FILE* input, row_evaluator row_evaluator, void* context);
int read_csv_row(FILE* input, int row_number, row_evaluator row_evaluator, void* context);

#endif // CSVFORMAT_H
