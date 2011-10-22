#!/bin/sh

./csvsel "select %3 where %2 = %3 or (%1 = Illinois or %1 = Washington)" < test.csv

# should print:
#
# Little Rock
# Denver
# Atlanta
# Honolulu
# Boise
# Chicago
# Indianapolis
# Des Moines
# Boston
# Oklahoma City
# Providence
# Columbia
# Salt Lake City
# Seattle
# Charleston
# Cheyenne

