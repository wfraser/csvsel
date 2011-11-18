/*
 * CSV Selector
 *
 * by William R. Fraser, 10/22/2011
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "growbuf.h"
#include "csvformat.h"

//#define DEBUG
#define DEBUG if (false)

/**
 * same as strchr() except it looks for multiple characters.
 *
 * Arguments:
 *   haystack	- string to be searched
 *   chars	- characters to search for
 *   nchars	- number of characters in chars
 *
 * Return Value:
 *   Pointer to the first one of chars in haystack, or NULL if none found.
 */
const char* strchrs(const char* haystack, const char* chars, size_t nchars)
{
    const char* end = haystack + strlen(haystack);
    while (haystack < end) {
        for (size_t i = 0; i < nchars; i++) {
            if (*haystack == chars[i]) {
                return haystack;
            }
        }
	haystack++;
    }

    return NULL;
}

/**
 * Print a CSV field, with appropriate double-quotes.
 *
 * No double-quotes are used, unless the field contains a comma, or a newline.
 *
 * Arguments:
 *   field	- field to print
 *   output	- file pointer to print to
 */
void print_csv_field(const char* field, FILE* output)
{
    if (NULL != strchrs(field, ",\n", 2)) {
        fprintf(output, "\"");
        for (size_t i = 0; i < strlen(field); i++) {
            if (field[i] == '"') {
                fprintf(output, "\"\"");
            }
            else {
                fprintf(output, "%c", field[i]);
            }
        }
        fprintf(output, "\"");
    }
    else {
        fprintf(output, "%s", field);
    }
}

/**
 * Read a CSV file, running a function on each row.
 *
 * Arguments:
 *   input	   - file pointer to CSV file to read
 *   row_evaluator - pointer to a function which takes 3 arguments:
 *                     - 2-dimensional growbuf with the fields
 *                     - the row number
 *                     - the context parameter passed to this function
 *                   and returns void.
 *   context       - arbitrary data to pass to the row evaluator
 */
int read_csv(FILE* input, row_evaluator row_evaluator, void* context)
{
    int retval = 0;
    
    growbuf* fields = NULL;
    growbuf* field  = NULL;
    
    size_t rownum = 0;
    bool in_dquot = false;
    bool prev_was_dquot = false;

    fields = growbuf_create(1);
    field = growbuf_create(32);
    growbuf_append(fields, &field, sizeof(growbuf*));
    while (true) { // iterate over lines
        int char_in = fgetc(input);

        if (EOF == char_in) {
            goto handle_eof;
        }

        char c = (char)char_in;
        switch (c) {
        case '"':
            if (in_dquot) {
                if (prev_was_dquot) {
                    growbuf_append(field, &c, 1);
                    prev_was_dquot = false;
                }
                else {
                    prev_was_dquot = true;
                    // don't append yet, wait for the next char.
                }
            }
            else if (field->size != 0) {
                growbuf_append(field, &c, 1);
            }
            else {
                in_dquot = true;
                // don't append.
           }
           break;

       case '\n':
           if (!in_dquot || prev_was_dquot) {
                // we're done with the line
                c = '\0';
                growbuf_append(field, &c, 1);

                DEBUG for (size_t i = 0; i < fields->size / sizeof(void*); i++)
                {
                    fprintf(stderr, "field %zu: ", i);
                    fprintf(stderr, "\"%s\"\n",
                        (char*)(((growbuf**)fields->buf)[i]->buf)
                    );
                }

                if (fields->size / sizeof(void*) > 0
                        && (fields->size / sizeof(void*) > 1
                            || ((growbuf**)fields->buf)[0]->size > 0))
                {
                    row_evaluator(fields, rownum, context);
                }

                for (size_t i = 0; i < fields->size / sizeof(void*); i++) {
                    growbuf_free(((growbuf**)(fields->buf))[i]);
                }
                fields->size = 0;
                field = growbuf_create(32);
                growbuf_append(fields, &field, sizeof(void*));
                in_dquot = false;
                prev_was_dquot = false;
                rownum++;
                break;
            }
            else {
                // embedded newline
                growbuf_append(field, &c, 1);
            }
            break;

        case ',':
            if (!in_dquot || prev_was_dquot) {
                // we're done with the field
                c = '\0';
                growbuf_append(field, &c, 1);

                field = growbuf_create(32);
                growbuf_append(fields, &field, sizeof(void*));
                in_dquot = false;
                prev_was_dquot = false;
            }
            else {
                growbuf_append(field, &c, 1);
            }
            break;

        default:
            if (in_dquot && prev_was_dquot) {
                fprintf(stderr, "csv format error: double-quoted field has "
                        "trailing garbage. Line %zu, field %zu\n",
                        rownum,
                        fields->size / sizeof(void*));
                retval = 1;
                goto cleanup;
            }
            else {
                growbuf_append(field, &c, 1);
            }
            break;

        } // switch

    } // while (true)

handle_eof:
    if (fields->size / sizeof(void*) > 0
            && ((fields->size / sizeof(void*) > 1 
                 || ((growbuf**)fields->buf)[0]->size > 0)))
    {
        row_evaluator(fields, rownum, context);
    }

cleanup:
    if (NULL != fields) {
        for (size_t i = 0; i < fields->size / sizeof(void*); i++) {
            growbuf_free(((growbuf**)(fields->buf))[i]);
        }
        growbuf_free(fields);
    }

    return retval;
}
