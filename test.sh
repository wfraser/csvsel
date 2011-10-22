#!/bin/sh

./csvsel "select %3 where %2 = %3 or (%1 = Wyoming or %1 = Washington)" < test.csv

