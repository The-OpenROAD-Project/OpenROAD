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
import argparse


def encode_file_to_base64_chunks(content, chunk_size=65536):
    encoded_content = base64.b64encode(content).decode("utf-8")
    chunks = [
        encoded_content[i : i + chunk_size]
        for i in range(0, len(encoded_content), chunk_size)
    ]
    return chunks


def join_files(file_list):
    combined_bytes = bytearray()

    for file_name in file_list:
        try:
            with open(file_name, "rb") as f:
                content = f.read()
                combined_bytes.extend(content)
        except Exception as e:
            print(f"Error reading {file_name}: {e}")

    return bytes(combined_bytes)


def main():
    parser = argparse.ArgumentParser(description="base64 encode a set of files.")

    parser.add_argument(
        "--inputs", "-i", nargs="+", required=True, help="List of input file names"
    )

    parser.add_argument("--output", "-o", required=True, help="Output file name")

    parser.add_argument("--namespace", "-n", required=True, help="Namespace name")

    parser.add_argument("--varname", "-v", required=True, help="Variable name")

    args = parser.parse_args()

    input_file = sys.argv[1]

    content = join_files(args.inputs)
    chunks = encode_file_to_base64_chunks(content)

    with open(args.output, "w") as out_file:
        out_file.write(f"namespace {args.namespace} {{\n")
        out_file.write(f"const char* {args.varname}[] = {{\n")
        for chunk in chunks:
            out_file.write(f'"{chunk}",\n')
        out_file.write(f"nullptr}};\n")
        out_file.write(f"}} // end namespace {args.namespace}\n")


if __name__ == "__main__":
    main()
