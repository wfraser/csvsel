#ifndef CSVSEL_UNITTEST_H
#define CSVSEL_UNITTEST_H

#include <stdbool.h>

bool test_dummy();

typedef struct {
    bool (*func)(void);
    const char* name;
} unittest;

unittest TESTS[] = {
    {test_dummy, "dummy test"}
};

#endif //CSVSEL_UNITTEST_H

