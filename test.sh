#!/bin/sh

valgrind --leak-check=full --show-reachable=yes ./csvsel -d -f test.csv select %1 where %2 = %3

