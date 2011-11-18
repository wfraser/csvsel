#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "growbuf.h"
#include "csvformat.h"

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
