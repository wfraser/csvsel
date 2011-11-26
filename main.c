/*
 * CSV Selector
 *
 * by William R. Fraser, 10/22/2011
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sysexits.h>
#include <string.h>
#include <errno.h>

#include "growbuf.h"
#include "csvformat.h"
#include "queryeval.h"
#include "util.h"
#include "csvsel.h"

#define DEBUG if (false)
//#define DEBUG

extern int query_debug;

int main(int argc, char** argv)
{
    int    retval          = EX_OK;
    FILE*  input           = stdin;
    size_t query_arg_start = 1;

    growbuf* query = growbuf_create(32);
    if (NULL == query) {
        fprintf(stderr, "malloc failed\n");
        retval = EX_OSERR;
        goto cleanup;
    }

    if (argc == 1) {
        fprintf(stderr, "usage: %s [-f inputfile] <query string>\n", argv[0]);
        retval = EX_USAGE;
        goto cleanup;
    }

    for (size_t i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0
                || strcmp(argv[i], "--debug") == 0) {
            query_debug = 1;
            query_arg_start = i + 1;
        }
        else if (strcmp(argv[i], "-f") == 0
                || strcmp(argv[i], "--file") == 0) {

            if (argc < 3) {
                fprintf(stderr, "%s flag needs an argument\n", argv[i]);
                retval = EX_USAGE;
                goto cleanup;
            }

            if (0 != access(argv[i + 1], R_OK)) {
                perror("unable to access input file");
                retval = EX_NOINPUT;
                goto cleanup;
            }

            if (input != NULL) {
                fclose(input);
            }

            input = fopen(argv[i + 1], "r");
            if (NULL == input) {
                perror("opening input failed");
                retval = EX_NOINPUT;
                goto cleanup;
            }

            i++;
            query_arg_start = i + 2;
        }
        else {
            break;
        }
    }

    //
    // accumulate all remaining command line arguments into one
    // space-separated string
    //

    const char* space = " ";
    for (size_t i = query_arg_start; i < argc; i++) {
        size_t len = strlen(argv[i]);
        growbuf_append(query, argv[i], len);
        growbuf_append(query, space, 1);
    }
    ((char*)query->buf)[ query->size - 1 ] = '\0';

    DEBUG printf("%s\n", (char*)query->buf);

    switch (csv_select(input, stdout, query->buf, query->size)) {
        case 0:
            retval = EX_OK;
            break;
        
        case 1:
            retval = EX_USAGE;
            break;

        default:
            retval = EX_OSERR;
            break;
    }

cleanup:
    if (NULL != query) {
        growbuf_free(query);
    }

    if (NULL != input) {
        fclose(input);
    }

    return retval;
}
