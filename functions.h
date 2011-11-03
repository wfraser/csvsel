#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdbool.h>
#include "queryparse.h"

#define MAX_ARGS 3
#define MAX_TYPES 3

typedef struct {
    size_t num_types;
    type types[MAX_TYPES];
} argument;

typedef struct {
    const char* name;
    type return_type;
    size_t num_args;
    size_t min_args;
    argument arguments[MAX_ARGS];
} functionspec;

#endif //FUNCTIONS_H
