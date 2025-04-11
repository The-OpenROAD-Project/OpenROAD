#!/usr/bin/env python3
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors
#
# base64 encode an input file and split it into string of length
# suitable for representing in a C string (64k).  Store the
# result in a nullptr terminated C array.

import base64
import sys
import json
import os


def encode_file_to_base64_chunks(file_path, chunk_size=65536):
    with open(file_path, "rb") as file:
        file_content = file.read()

    encoded_content = base64.b64encode(file_content).decode("utf-8")
    chunks = [
        encoded_content[i : i + chunk_size]
        for i in range(0, len(encoded_content), chunk_size)
    ]
    return chunks


def main():
    if len(sys.argv) != 5:
        print(
            "Usage: python script.py <input_file> <output_file>"
            " <var_name> <namespace>"
        )
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    var_name = sys.argv[3]
    namespace = sys.argv[4]

    if not os.path.isfile(input_file):
        print(f"Error: File '{input_file}' does not exist.")
        sys.exit(1)

    chunks = encode_file_to_base64_chunks(input_file)

    with open(output_file, "w") as out_file:
        out_file.write(f"namespace {namespace} {{\n")
        out_file.write(f"const char* {var_name}[] = {{\n")
        for chunk in chunks:
            out_file.write(f'"{chunk}",\n')
        out_file.write(f"nullptr}};\n")
        out_file.write(f"}} // end namespace {namespace}\n")


if __name__ == "__main__":
    main()
