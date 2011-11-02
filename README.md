CSV Select
==========

CSV Select lets you do SQL-esque queries against CSV (comma-separated-values)
data.

Build Dependencies
------------------

* GNU Flex
* GNU Bison
* GCC with C99 support

Query Language
--------------

    query: select [<columns>] [where <conditions>]

    columns: column[, columns]      (plurality of columns)
    columns: column[ - columns]     (range of columns)
                                    (these can be mixed)

    column: %<digits>[(.dbl | .num)]

    conditions: [(] [not] <condition> [ (and|or) <conditions ] [)]

    condition: <value> <operator> <value>

    value: (<column> | <special> | "string" | number)

    operator: (= | != | < | > | <= | >= )

    special:
        %#  (number of the current row, 0-based)
        %%  (total number of columns in the current row)

Notes
-----

Values have types, either string, integer ("num"), or floating-point ("dbl").

Columns are automatically string type, unless you add ".dbl" or ".num" after them.

The special values %# and %% are integers.

Constant numbers are integers unless they have a decimal point in them, in which case they are floating-point.

Comparisons are done as follows:

* string vs string: ordinal string comparison
* string vs dbl: string is parsed to double, then numeric comparison
* string vs num: string is parsed to long, then numeric comparison
* dbl vs num: num is upgraded to a dbl, then numeric comparison
* dbl vs dbl, and num vs num: numeric comparison

If a string to double or string to long conversion fails, the numeric value of the string is zero.

Examples
--------

Print the first column of any row where the second and third columns are equal (string comparison):

    select %1 where %2 = %3

Print the first column of any row where the second column (as a floating point number) is greater than 25.5:

    select %1 where %2.dbl > 25.5

Print all the columns of rows 25 through 42:

    select where %# >= 25 and %# <= 42

Just print the second row of the whole file:
    
    select %2

License
-------

CSV Select is copyright (c) 2011 William R. Fraser

CSV Select is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
