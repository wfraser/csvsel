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
#include "csvsel.h"
#include "util.h"

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
    char buf[512] = {0};
    ssize_t bytes;
    do {
        bytes = read(fd, buf, sizeof(buf));
        if (bytes > 0) {
            growbuf_append(out, buf, bytes);
        }
    } while (bytes > 0);
}

enum { READ = 0, WRITE = 1 };

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
    FILE* print_output;

    pipe(fd);
    
    // set our read end nonblocking because csvsel won't close it
    fcntl(fd[READ], F_SETFL, O_NONBLOCK);

    print_output = fdopen(fd[WRITE], "a");
    print_csv_field(instr, print_output);
    fflush(print_output);

    buf = growbuf_create(10);
    read_fd(fd[READ], buf);

    //printf("%1$d bytes: (%2$.*1$s)\n", buf->size, (char*)buf->buf);

    if (0 == strncmp(outstr, (char*)buf->buf, strlen(outstr))) {
        retval = true;
    }

    growbuf_free(buf);
    fclose(print_output);
    close(fd[WRITE]);
    close(fd[READ]);

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
    bool oks[11] = {0};

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

    return retval;
}

bool test_substr()
{
    bool ret = false;
    growbuf* fields = growbuf_create(1*sizeof(void*));
    growbuf* field = growbuf_create(0);
    growbuf_append(fields, &field, sizeof(void*));
    field->buf = (void*)"graycode";
    
    val value = {0};
    func function = {0};

    value.is_func = true;
    value.func = &function;
    value.conversion_type = TYPE_STRING;

#define TYPE_CHECKS(x) (x.is_str && x.conversion_type == TYPE_STRING \
        && NULL != x.str)

    function.func = FUNC_SUBSTR;
    function.args[0].is_col = true;
    function.args[0].col = 0;
    function.args[0].conversion_type = TYPE_STRING;
    function.args[1].is_num = true;
    function.args[1].conversion_type = TYPE_LONG;
    function.num_args = 2;

    function.args[1].num = 20;
    val final = value_evaluate(&value, fields, 0);

    // 20 from the start (start out of range => empty string)
    // substr("graycode", 20) = ""
    if (!TYPE_CHECKS(final) || strcmp("", final.str) != 0) {
        printf("#1\n");
        print_val(final);
        goto cleanup;
    }

    free(final.str);
    function.args[1].num = -3;
    final = value_evaluate(&value, fields, 0);

    // 3 from the end, to the end
    // substr("graycode", -3) = "ode"
    if (!TYPE_CHECKS(final) || strcmp("ode", final.str) != 0) {
        printf("#2\n");
        print_val(final);
        goto cleanup;
    }

    free(final.str);
    function.args[1].num = 4;
    final = value_evaluate(&value, fields, 0);

    // 4 from the start, to the end
    // substr("graycode", 4) = "code"
    if (!TYPE_CHECKS(final) || strcmp("code", final.str) != 0) {
        printf("#3\n");
        print_val(final);
        goto cleanup;
    }

    free(final.str);
    function.args[2].is_num = true;
    function.args[2].num = 3;
    function.args[2].conversion_type = TYPE_LONG;
    function.num_args = 3;
    final = value_evaluate(&value, fields, 0);

    // 4 from the start, length of 3
    // substr("graycode", 4, 3) = "cod"
    if (!TYPE_CHECKS(final) || strcmp("cod", final.str) != 0) {
        printf("#4\n");
        print_val(final);
        goto cleanup;
    }

    free(final.str);
    function.args[2].num = -3;
    final = value_evaluate(&value, fields, 0);
  
    // 4 from the start, up to and including 3 from the end
    // substr("graycode", 4, -3) = "co"
    if (!TYPE_CHECKS(final) || strcmp("co", final.str) != 0) {
        printf("#5\n");
        print_val(final);
        goto cleanup;
    }

    free(final.str);
    function.args[1].num = 20;
    final = value_evaluate(&value, fields, 0);

    // 20 from the start, up to and including 3 from the end
    // doesn't make sense because start > end, so yields empty string
    // substr("graycode", 7, -3) = ""
    if (!TYPE_CHECKS(final) || strcmp("", final.str) != 0) {
        printf("#6\n");
        print_val(final);
        goto cleanup;
    }

    free(final.str);
    function.args[1].num = -20;
    function.args[2].num = 40;
    final = value_evaluate(&value, fields, 0);

    // 20 from the end, length of 40.
    // out of bounds start gets trimmed to 0
    // out of bounds length gets trimmed to length of the string
    // substr("graycode", -20, 40) = "graycode"
    if (!TYPE_CHECKS(final) || strcmp("graycode", final.str) != 0) {
        printf("#7\n");
        print_val(final);
        goto cleanup;
    }

    free(final.str);
    ret = true;

cleanup:
    field->buf = NULL;
    growbuf_free(field);
    growbuf_free(fields);

    return ret;
}

bool test_upper_lower()
{
    bool ret = false;
    growbuf* fields = growbuf_create(1*sizeof(void*));
    growbuf* field = growbuf_create(0);
    growbuf_append(fields, &field, sizeof(void*));
    val value = {0};
    func function = {0};

    value.is_func = true;
    value.func = &function;
    value.conversion_type = TYPE_STRING;

    function.func = FUNC_UPPER;
    function.args[0].is_col = true;
    function.args[0].col = 0;
    function.args[0].conversion_type = TYPE_STRING;
    function.num_args = 1;

    field->buf = "gRaYcOde12 34_-+";
    
    val final = value_evaluate(&value, fields, 0);

    if (!TYPE_CHECKS(final) || strcmp("GRAYCODE12 34_-+", final.str) != 0) {
        printf("upper failed\n");  
        print_val(final);
        goto cleanup;
    }  

    free(final.str);
    function.func = FUNC_LOWER;
    final = value_evaluate(&value, fields, 0);

    if (!TYPE_CHECKS(final) || strcmp("graycode12 34_-+", final.str) != 0) {
        printf("lower failed\n");
        print_val(final);
        goto cleanup;
    }

    free(final.str);
    ret = true;

cleanup:
    field->buf = NULL;
    growbuf_free(field);
    growbuf_free(fields);

    return ret;
}

