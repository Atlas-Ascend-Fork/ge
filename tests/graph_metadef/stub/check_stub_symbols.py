#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

"""Check that generated stub files keep the required symbols.

The stub libgraph.so must cover all symbols declared by public headers,
otherwise samples linking against devlib break at link time (undefined
reference). This script asserts the presence of known-critical snippets
in the generated stub sources and fails the build when any is missing.
"""

import sys

REQUIRED_SNIPPETS = {
    "stub_ascend_string.cc": [
        "bool AscendString::operator<(const AscendString &d) const",
        "bool AscendString::operator>(const AscendString &d) const",
        "bool AscendString::operator<=(const AscendString &d) const",
        "bool AscendString::operator>=(const AscendString &d) const",
        "(d.name_ != nullptr)",
        "return d < *this;",
        "return !(d < *this);",
        "return !(*this < d);",
    ],
}


def main():
    if len(sys.argv) < 2:
        print("ERROR: usage: {} <stub_output_dir>".format(sys.argv[0]))
        return 1
    out_dir = sys.argv[1].rstrip("/")
    failed = False
    for file_name, snippets in REQUIRED_SNIPPETS.items():
        with open(out_dir + "/" + file_name) as stub_file:
            content = stub_file.read()
        for snippet in snippets:
            if snippet not in content:
                print(
                    "ERROR: {} missing required snippet: [{}]".format(
                        file_name, snippet
                    )
                )
                failed = True
    if failed:
        return 1
    print("Stub symbol check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
