#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "growbuf.h"
#include "queryparse.h"
#include "queryparse.tab.h"
#include "util.h"
#include "unittests.h"

extern int query_debug;

void test_parse(const char* query)
{
    growbuf* selectors = growbuf_create(1);
    compound *condition = NULL;

    printf(">>> query: %1$s\n", query);

    if (0 != queryparse(query, strlen(query), selectors, &condition)) {
        printf(">>> parse error(s)\n");
    }
    else {
        printf(">>> parse ok\n");
    }

    for (size_t i = 0; i < (selectors->size / sizeof(void*)); i++) {
        selector* s = ((selector**)(selectors->buf))[i];
        printf(">>> selected ");
        print_selector(s);
    }

    if (NULL != condition) {
        printf(">>> condition:\n");
        print_condition(condition, 0);
    }
    else {
        printf(">>> no condition\n");
    }
}

extern unittest TESTS[];

bool do_test(size_t test_num)
{
    return (*TESTS[test_num].func)();
}

int main(int argc, char** argv)
{
    int retval = 0;

    if (argc == 1) {
        fprintf(stderr, "csvsel tester\nusage: %1$s <test number or \"all\" or \"parse\">\n",
                argv[0]);
        retval = -1;
        goto cleanup;
    }

    if (strcmp("parse", argv[1]) == 0) {
        if (argc != 3) {
            fprintf(stderr, "usage error: need query string as 3rd argument\n");
            retval = -1;
            goto cleanup;
        }

        test_parse(argv[2]);
    }
    else if (strcmp("all", argv[1]) == 0) {
        for (size_t test_num = 0; 
                test_num < sizeof(TESTS) / sizeof(TESTS[0]);
                test_num++) {

            bool pass = do_test(test_num);
            if (!pass) {
                retval = 1;
            }

            printf("test %1$03zu: %3$s -- %2$s\n",
                    test_num,
                    TESTS[test_num].name,
                    pass ? "pass" : "FAIL");
        }
    }
    else {
        size_t test_num = (size_t)atol(argv[1]);
        size_t max_test = sizeof(TESTS) / sizeof(TESTS[0]);

        if (test_num >= max_test) {
            fprintf(stderr, "specified test number (%1$zu) is out of range.\n"
                    " It must be between 0 and %2$zu.\n",
                    test_num,
                    max_test);
            retval = -2;
            goto cleanup;
        }
        else {
            bool pass = do_test(test_num);
            if (!pass) {
                retval = 1;
            }

            printf("test %1$03zu: %3$s -- %2$s\n",
                    test_num,
                    TESTS[test_num].name,
                    pass ? "pass" : "FAIL");
        }
    }

cleanup:
    return retval;
}
