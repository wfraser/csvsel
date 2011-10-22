#ifndef QUERY_H
#define QUERY_H

#include "growbuf.h"

int queryparse(const char* query, size_t query_length, growbuf* selected_columns, growbuf* conditions);

#endif // QUERY_H
