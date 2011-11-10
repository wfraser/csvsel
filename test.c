#include <stdio.h>
#include <string.h>
#include "growbuf.h"
#include "queryparse.h"
#include "queryparse.tab.h"
#include "util.h"

int query_debug = 1;

int main(int argc, char** argv)
{
    growbuf* selectors = growbuf_create(1);
    compound *condition = NULL;

    if (argc == 1) {
        printf("query parser test\nusage: test <query>\n");
        return 1;
    }
    else {
        if (0 != queryparse(argv[1], strlen(argv[1]), selectors, &condition)) {
            printf(">>> parse error(s)\n");
        }
        else {
            printf(">>> parse ok\n");
        }
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

    return 0;
}
