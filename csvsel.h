#ifndef CSVSEL_H
#define CSVSEL_H

#include <stdio.h>

int csv_select(FILE* input, const char* query, size_t query_length);

#endif // CSVSEL_H
