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

val value_evaluate(const val* val, growbuf* fields, size_t rownum);

bool query_evaluate(growbuf* fields, compound* condition, size_t rownum);

#endif //QUERYEVAL_H
