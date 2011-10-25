#ifndef QUERYPARSE_UTIL_H
#define QUERYPARSE_UTIL_H

#include "queryparse.h"

void print_rval(rval r);
void print_indent(size_t indent);
void print_condition(compound* c, size_t indent);

#endif //QUERYPARSE_UTIL_H
