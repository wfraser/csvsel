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

    query: select [<selectors>] [where <conditions>]

    selectors: [columns,values]

    columns: column[, columns]      (plurality of columns)
    columns: column[ - columns]     (range of columns)
                                    (these can be mixed)

    column: %<digits>

    conditions: [(] [not] <condition> [ (and|or) <conditions ] [)]

    condition: <value> <operator> <value>

    value: (<column> | <special> | "string" | number | <function>(value, ...) )[(.float | .int | .string)]

    operator: (= | != | < | > | <= | >= )

    special:
        %#  (number of the current row, 0-based)
        %%  (total number of columns in the current row)

Functions
---------

* **`substr`** ( string `s`, int `start`, int `length` *(default -1)* ) -> string
    * returns the substring of `s` starting at `start`, and of length `length`
    * if `length` is negative, it counts from the end of the string. -1 means "to the end of the string".

* **`strlen`** ( string `s` ) -> int
    * returns the length of string `s`

* **`min`** / **`max`** ( int|float `a`, int|float `b` ) -> float
    * returns the smaller or bigger, respectively, of `a` and `b`, as a float.

Notes
-----

Values have types, either string, integer ("int"), or floating-point ("float").

Columns are automatically string type, unless you add ".int" or ".float" after them.

The special values %# and %% are integers.

Constant numbers are integers unless they have a decimal point in them, in which case they are floating-point.

Functions return different types depending on the function.

Any value can be converted to another type by adding ".int", ".float", or ".string" after it.

When doing the comparisons, automatic type promotions are done as follows:

* string vs string: ordinal string comparison (no promotion)
* string vs float: string is parsed to double, then numeric comparison
* string vs int: if the string contains a dot, string is parsed to double, otherwise string is parsed to int (long), then numeric comparison
* float vs int: int is upgraded to a double, then numeric comparison
* float vs float, and int vs int: numeric comparison

If a string to double or string to long conversion fails, the numeric value of the string is zero.

Examples
--------

Print the first column of any row where the second and third columns are equal (string comparison):

    select %1 where %2 = %3

Print columns 1 thru 3 of any row where the second column (as a floating point number) is greater than 25.5:

    select %1-%3 where %2.float > 25.5

Print all the columns of rows 25 through 42:

    select where %# >= 25 and %# <= 42

Print the 2nd, 4th, 5th, and 6th columns of the whole file:
    
    select %2,%4-%6

Print the first column of all rows where the 2nd column is longer than the 3rd:

    select %1 where strlen(%2) > strlen(%3)

Print the length of the first column of all rows:

    select strlen(%1)

License
-------

CSV Select is copyright (c) 2011 William R. Fraser

CSV Select is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
