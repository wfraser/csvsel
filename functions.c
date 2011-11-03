#include "functions.h"

functionspec FUNCTIONS[] = {
    {
        .name = "substr",
        .return_type = TYPE_STRING,
        .num_args = 3,
        .min_args = 2,
        .arguments = {
            {
                .num_types = 1,
                .types = { TYPE_STRING }
            },
            {
                .num_types = 1,
                .types = { TYPE_LONG }
            },
            {
                .num_types = 1,
                .types = { TYPE_LONG }
            }
        }
    },
    {
        .name = "strlen",
        .return_type = TYPE_LONG,
        .num_args = 1,
        .min_args = 1,
        .arguments = {
            {
                .num_types = 1,
                .types = { TYPE_STRING }
            }
        }
    },
    {
        .name = "max",
        .return_type = TYPE_DOUBLE,
        .num_args = 2,
        .min_args = 2,
        .arguments = {
            {
                .num_types = 2,
                .types = { TYPE_LONG, TYPE_DOUBLE }
            },
            {
                .num_types = 2,
                .types = { TYPE_LONG, TYPE_DOUBLE }
            },
        }
    },
    {
        .name = "min",
        .return_type = TYPE_DOUBLE,
        .num_args = 2,
        .min_args = 2,
        .arguments = {
            {
                .num_types = 2,
                .types = { TYPE_LONG, TYPE_DOUBLE }
            },
            {
                .num_types = 2,
                .types = { TYPE_LONG, TYPE_DOUBLE }
            }
        }
    }
};

