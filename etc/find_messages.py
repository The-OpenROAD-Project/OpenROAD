#!/usr/bin/env python3

############################################################################
##
## Copyright (c) 2021, The Regents of the University of California
## All rights reserved.
##
## BSD 3-Clause License
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
############################################################################

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
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "-d",
        "--dir",
        default=os.getcwd(),
        help="Directory to start the search for messages from"
    )
    parser.add_argument(
        "-l",
        "--local",
        action='store_true',
        help="Look only at the local files and don't recurse"
    )
    args = parser.parse_args()

    return args

# The three capture groups are tool, id, and message.
warn_regexp_c = \
    re.compile(r'''
      (?:->|\.)                                        # deref
      (?:info|warn|fileWarn|error|fileError|critical)  # type
      \s*\(\s*                                         # (
      (?:utl::)?(?P<tool>[A-Z]{3})                     # tool
      \s*,\s*                                          # ,
      (?P<id>\d+)                                      # id
      \s*,\s*                                          # ,
      "(?P<message>(?:[^"\\]|\\.)+?)"                  # message
    ''', re.VERBOSE | re.MULTILINE)

warn_regexp_tcl = \
    re.compile(r'''
      (?:info|warn|error|critical)     # type
      \s+                              # white-space
      (?P<tool>[A-Z]{3}|"[A-Z]{3}")    # tool
      \s+                              # white-space
      (?P<id>\d+)                      # id
      \s+                              # white-space
      "(?P<message>(?:[^"\\]|\\.)+?)"  # message
    ''', re.VERBOSE | re.MULTILINE)

def scan_file(path, file_name, msgs):
    # Grab the file contents as a single string
    with open(os.path.join(path, file_name), encoding='utf-8') as file_handle:
        lines = file_handle.read()

    warn_regexp = warn_regexp_tcl if file_name.endswith('tcl') \
        else warn_regexp_c

    for match in  re.finditer(warn_regexp, lines):
        tool = match.group('tool').strip('"')
        msg_id = int(match.group('id'))
        key = '{} {:04d}'.format(tool, msg_id)

        # Count the newlines before the match starts
        line_num = lines[0:match.start()].count('\n') + 1
        position = '{}:{}'.format(file_name, line_num)
        value = '{:25} {}'.format(position, match.group('message'))

        msgs[key].add(value)

def scan_dir(path, files, msgs):
    for file_name in files:
        if re.search(r'\.(c|cc|cpp|cxx|h|hh|yy|ll|i|tcl)$', file_name):
            scan_file(path, file_name, msgs)

def main():
    args = parse_args()

    # "tool id" -> "file:line message"
    msgs = defaultdict(set)

    if args.local: # no recursion
        files = [os.path.basename(file)
                 for file in glob.glob(os.path.join(args.dir, '*'))]
        scan_dir(args.dir, files, msgs)
    else:
        for path, _, files in os.walk(args.dir):
            scan_dir(path, files, msgs)

    has_error = False
    for key in sorted(msgs):
        count = len(msgs[key])
        if count > 1:
            print('Error: {} used {} times'.format(key, count),
                  file=sys.stderr)
            has_error = True

    for key in sorted(msgs):
        for msg in sorted(msgs[key]):
            print(key, msg)

    if has_error:
        sys.exit(1)

if __name__ == "__main__":
    main()
