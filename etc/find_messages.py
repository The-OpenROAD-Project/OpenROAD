#!/usr/bin/env python3

## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2021-2026, The OpenROAD Authors

# Usage:
# cd src/<tool>
# ../../etc/FindMessages.py > messages.txt

import argparse
import glob
import os
import re
import sys
from collections import defaultdict


def parse_args():
    parser = argparse.ArgumentParser(
        description="""
          Find logger calls and report sorted message IDs.
          Also checks for duplicate message IDs.
        """,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "-d",
        "--dir",
        default=os.getcwd(),
        help="Directory to start the search for messages from",
    )
    parser.add_argument(
        "-l",
        "--local",
        action="store_true",
        help="Look only at the local files and don't recurse",
    )
    args = parser.parse_args()

    return args


# The three capture groups are tool, id, and message.
warn_regexp_c = re.compile(
    r"""
      (?:->|\.)                                        # deref
      (?P<type>info|warn|fileWarn|error|fileError|critical)  # type
      \s*\(\s*                                         # (
      (?:utl::)?(?P<tool>[A-Z]{3})                     # tool
      \s*,\s*                                          # ,
      (?P<id>\d+)                                      # id
      \s*,\s*                                          # ,
      (?P<message>("((?:[^"\\]|\\.)+?\s*)+" ))          # message
    """,
    re.VERBOSE | re.MULTILINE,
)


warn_regexp_tcl = re.compile(
    r"""
      (?P<type>info|warn|error|critical)     # type
      \s+                              # white-space
      (?P<tool>[A-Z]{3}|"[A-Z]{3}")    # tool
      \s+                              # white-space
      (?P<id>\d+)                      # id
      \s+                              # white-space
      (?P<message>"(?:[^"]|\\.)+?")  # message
    """,
    re.VERBOSE | re.MULTILINE,
)


def scan_file(path, file_name, msgs):
    # Grab the file contents as a single string
    with open(os.path.join(path, file_name), encoding="utf-8") as file_handle:
        lines = file_handle.read()

    warn_regexp = warn_regexp_tcl if file_name.endswith("tcl") else warn_regexp_c

    for match in re.finditer(warn_regexp, lines):
        tool = match.group("tool").strip('"')
        msg_id = int(match.group("id"))
        key = "{} {:04d}".format(tool, msg_id)

        # remove quotes and join strings
        message = match.group("message")
        message = message.replace("\n", "")
        message = message.rstrip()[1:-1]
        message = re.sub(r'"\s*"', "", message)
        message_type = match.group("type").upper()

        # Count the newlines before the match starts
        line_num = lines[0 : match.start()].count("\n") + 1
        position = "{}:{}".format(file_name, line_num)
        file_link = os.path.join(path, file_name).strip("../").replace("\\", "/")
        file_link = "https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/{}#L{}".format(
            file_link, line_num
        )
        value = "{:25} {} {} {}".format(position, message, message_type, file_link)

        msgs[key].add(value)


def scan_dir(path, files, msgs):
    for file_name in files:
        if re.search(r"\.(c|cc|cpp|cxx|h|hh|yy|ll|i|tcl)$", file_name):
            scan_file(path, file_name, msgs)


def main():
    args = parse_args()

    # "tool id" -> "file:line message"
    msgs = defaultdict(set)

    if args.local:  # no recursion
        files = [
            os.path.basename(file) for file in glob.glob(os.path.join(args.dir, "*"))
        ]
        scan_dir(args.dir, files, msgs)
    else:
        for path, _, files in os.walk(args.dir):
            scan_dir(path, files, msgs)

    # Group numbers by set name
    set_numbers = defaultdict(set)
    for key in msgs:
        set_name, number = key.split()
        set_numbers[set_name].add(int(number))

    has_error = False
    for key in sorted(msgs):
        count = len(msgs[key])
        if count > 1:
            set_name, number = key.split()
            next_free_integer = int(number) + 1
            while next_free_integer in set_numbers[set_name]:
                next_free_integer += 1
            print(
                "Error: {} used {} times, next free message id is {}".format(
                    key, count, next_free_integer
                ),
                file=sys.stderr,
            )
            for idloc in sorted(msgs[key]):
                fileloc, *_ = idloc.split()
                file, line = fileloc.split(":")
                print(
                    "  Appears in {} on line {} ".format(file, line),
                    file=sys.stderr,
                )
            has_error = True

    for key in sorted(msgs):
        for msg in sorted(msgs[key]):
            print(key, msg)

    if has_error:
        sys.exit(1)


if __name__ == "__main__":
    main()
