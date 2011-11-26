/*
 * CSV Selector
 *
 * by William R. Fraser, 10/22/2011
 */

#ifndef QUERYEVAL_H
#define QUERYEVAL_H

#include <stdbool.h>

#include "growbuf.h"
#include "queryparse.h"

void val_free(val* val);
void selector_free(selector* s);

val value_evaluate(const val* val, growbuf* fields, size_t rownum);

bool query_evaluate(growbuf* fields, size_t rownum, compound* condition);

#endif //QUERYEVAL_H
