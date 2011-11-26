#ifndef CSVSEL_UNITTEST_H
#define CSVSEL_UNITTEST_H

#include <stdbool.h>

bool test_dummy();
bool test_csv_out1();
bool test_csv_out2();
bool test_csv_out3();
bool test_csv_out4();
bool test_select_columns();
bool test_substr();
bool test_upper_lower();

typedef struct {
    bool (*func)(void);
    const char* name;
} unittest;

unittest TESTS[] = {
    {test_dummy,    "dummy test"},
    {test_csv_out1, "csv output #1 (comma)"},
    {test_csv_out2, "csv output #2 (dquote)"},
    {test_csv_out3, "csv output #3 (space)"},
    {test_csv_out4, "csv output #4 (newline)"},
    {test_select_columns,   "select columns"},
    {test_substr,   "substr()"},
    {test_upper_lower,      "upper() and lower()"},
};

#endif //CSVSEL_UNITTEST_H

