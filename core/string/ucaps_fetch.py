"""
Script used to dump case mappings
from the Unicode Character Database
to the `ucaps.h` file.
"""

import os
from typing import List, Tuple
from urllib.request import urlopen

URL = "https://www.unicode.org/Public/16.0.0/ucd/UnicodeData.txt"


lines = [line.decode("utf-8") for line in urlopen(URL)]

lower_to_upper: List[Tuple[str, str]] = []
upper_to_lower: List[Tuple[str, str]] = []
for line in lines:
    split_line = line.split(";")

    code_value = split_line[0].strip()
    uppercase_mapping = split_line[12].strip()
    lowercase_mapping = split_line[13].strip()

    if uppercase_mapping:
        lower_to_upper.append((f"0x{code_value}", f"0x{uppercase_mapping}"))
    if lowercase_mapping:
        upper_to_lower.append((f"0x{code_value}", f"0x{lowercase_mapping}"))
ltu_len = len(lower_to_upper)
utl_len = len(upper_to_lower)


ucaps_str = f"""/**************************************************************************/
/*  ucaps.h                                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef UCAPS_H
#define UCAPS_H

#define LTU_LEN {ltu_len}
#define UTL_LEN {utl_len}

static const int caps_table[LTU_LEN][2] = {{
\t"""

for lower, upper in lower_to_upper:
    ucaps_str += f"{{ {lower}, {upper} }},\n\t"

ucaps_str = ucaps_str[:-1]  # Remove trailing tab.
ucaps_str += """};

static const int reverse_caps_table[UTL_LEN][2] = {
\t"""

for upper, lower in upper_to_lower:
    ucaps_str += f"{{ {upper}, {lower} }},\n\t"

ucaps_str = ucaps_str[:-1]  # Remove trailing tab.
ucaps_str += """};

static int _find_upper(int ch) {
\tint low = 0;
\tint high = LTU_LEN - 1;
\tint middle;

\twhile (low <= high) {
\t\tmiddle = (low + high) / 2;

\t\tif (ch < caps_table[middle][0]) {
\t\t\thigh = middle - 1; // Search low end of array.
\t\t} else if (caps_table[middle][0] < ch) {
\t\t\tlow = middle + 1; // Search high end of array.
\t\t} else {
\t\t\treturn caps_table[middle][1];
\t\t}
\t}

\treturn ch;
}

static int _find_lower(int ch) {
\tint low = 0;
\tint high = UTL_LEN - 1;
\tint middle;

\twhile (low <= high) {
\t\tmiddle = (low + high) / 2;

\t\tif (ch < reverse_caps_table[middle][0]) {
\t\t\thigh = middle - 1; // Search low end of array.
\t\t} else if (reverse_caps_table[middle][0] < ch) {
\t\t\tlow = middle + 1; // Search high end of array.
\t\t} else {
\t\t\treturn reverse_caps_table[middle][1];
\t\t}
\t}

\treturn ch;
}

#endif // UCAPS_H
"""

ucaps_path = os.path.join(os.path.dirname(__file__), "ucaps.h")
with open(ucaps_path, "w", newline="\n") as f:
    f.write(ucaps_str)

print("`ucaps.h` generated successfully.")
