#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "growbuf.h"
#include "csvformat.h"
#include "queryparse.h"

extern int query_debug;

/**
 * Read the contents of a file descriptor into a growbuf.
 *
 * Arguments:
 *   fd     - file descriptor number to read from
 *   out    - growbuf to append to
 */
void read_fd(int fd, growbuf* out)
{
    char buf[512] = {};
    ssize_t bytes;
    do {
        bytes = read(fd, buf, sizeof(buf));
        if (bytes > 0) {
            growbuf_append(out, buf, bytes);
        }
    } while (bytes > 0);
}

enum { IN = 0, OUT = 1};

bool test_dummy()
{
    return true;
}

/**
 * Test harness for print_csv_field()
 *
 * Arguments:
 *   instr  - data to print with print_csv_field()
 *   outstr - expected output
 *
 * Returns:
 *   true if print_csv_field()'s output matches outstr,
 *   false otherwise.
 */
bool test_csv_out(const char* instr, const char* outstr)
{
    bool retval = false;
    int fd[2];
    growbuf* buf;
    FILE* out;

    pipe(fd);
    fcntl(fd[IN], F_SETFL, O_NONBLOCK);
    out = fdopen(fd[OUT], "a");

    print_csv_field(instr, out);
    fflush(out);
    buf = growbuf_create(10);
    read_fd(fd[IN], buf);

    //printf("%1$d bytes: (%2$.*1$s)\n", buf->size, (char*)buf->buf);

    if (0 == strncmp(outstr, (char*)buf->buf, strlen(outstr))) {
        retval = true;
    }

    growbuf_free(buf);
    fclose(out);
    close(fd[IN]);
    close(fd[OUT]);

    return retval;
}

bool test_csv_out1()
{
    return test_csv_out("hello,world", "\"hello,world\"");
}

bool test_csv_out2()
{
    return test_csv_out("hello\"world", "hello\"world");
}

bool test_csv_out3()
{
    return test_csv_out("hello world", "hello world");
}

bool test_csv_out4()
{
    return test_csv_out("hello\nworld", "\"hello\nworld\"");
}

/**
 * Select columns 1-10 and 77, specifying some overlaps too.
 * Tests that the overlap detection works, as well as basic column selection.
 */
bool test_select_columns()
{
    bool retval = false;
    growbuf* selected_columns = growbuf_create(10);
    compound* root_condition = NULL;
    const char* query = "select %1-%10,%7,%77,%2-%4";
    selector** selectors = NULL;
    bool oks[11] = {};

    query_debug = 0;
    if (0 != queryparse(query, strlen(query), selected_columns, &root_condition)) {
        retval = false;
        printf("parse failed\n");
        goto cleanup;
    }

    if (11 != selected_columns->size / sizeof(void*)) {
        retval = false;
        printf("wrong number of cols selected\n");
        goto cleanup;
    }

    selectors = (selector**)selected_columns->buf;

    for (size_t i = 0; i < 11; i++) {
        if (selectors[i]->type != SELECTOR_COLUMN) {
            retval = false;
            printf("selected something not a column\n");
            goto cleanup;
        }
        if (selectors[i]->column == 76) {
            oks[10] = true;
        }
        else if (selectors[i]->column <= 10) {
            oks[selectors[i]->column] = true;
        }
        else {
            retval = false;
            printf("selected a column not in the set: %zu\n",
                    selectors[i]->column);
            goto cleanup;
        }
    }

    for (size_t i = 0; i < 11; i++) {
        if (!oks[i]) {
            retval = false;
            printf("a column missing from the set: %zu\n", i);
            goto cleanup;
        }
    }

    retval = true;

cleanup:
    if (selected_columns != NULL) {
        for (size_t i = 0; i < selected_columns->size / sizeof(void*); i++) {
            free(((selector**)selected_columns->buf)[i]);
        }
        growbuf_free(selected_columns);
    }

    if (root_condition != NULL) {
        free(root_condition);
    }

    query_debug = 1;

    return retval;
}
