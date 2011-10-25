#!/bin/sh

valgrind ./csvsel -d -f test.csv select %1 where %2 = %3

