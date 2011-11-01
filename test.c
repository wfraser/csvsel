#include <stdio.h>
#include <string.h>
#include "growbuf.h"
#include "queryparse.h"
#include "queryparse.tab.h"
#include "util.h"

int query_debug = 1;

int main(int argc, char** argv)
{
    growbuf* sc = growbuf_create(1);
    compound *c;

    //queryparse(NULL, 0, sc, &c);
    if (argc == 1) {
        printf("query parser test\nusage: test <query>\n");
        return 1;
    }
    else {
        queryparse(argv[1], strlen(argv[1]), sc, &c);
    }

    for (size_t i = 0; i < (sc->size / sizeof(long)); i++) {
        printf("selected column %ld\n", ((long*)sc->buf)[i]);
    }

    if (NULL != c) {
        printf("condition:\n");
        print_condition(c, 0);
    }

    return 0;
}
