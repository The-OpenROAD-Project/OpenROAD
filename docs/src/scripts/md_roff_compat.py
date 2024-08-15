###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2024, The Regents of the University of California
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
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
###############################################################################

# This code contains the scripts to convert the individual module READMEs
#  into individual functions for man2 and man3 level.

import os
from manpage import ManPage
from extract_utils import extract_tcl_command, extract_description
from extract_utils import extract_tcl_code, extract_arguments
from extract_utils import extract_tables, parse_switch

# Undocumented manpages.
# sta: documentation is hosted elsewhere. (not currently in RTD also.)
# odb: documentation is hosted on doxygen.

tools = [
    "ant",
    "cts",
    "dbSta",
    "dft",
    "dpl",
    "dpo",
    "drt",
    "dst",
    "fin",
    "gpl",
    "grt",
    "gui",
    "ifp",
    "mpl",
    "mpl2",
    "odb",
    "pad",
    "par",
    "pdn",
    "ppl",
    "psm",
    "rcx",
    "rmp",
    "rsz",
    "sta",
    "stt",
    "tap",
    "upf",
    "utl",
]

# Process man2 (except odb and sta)
DEST_DIR2 = SRC_DIR = "./md/man2"
exclude2 = ["odb", "sta"]
docs2 = [f"{SRC_DIR}/{tool}.md" for tool in tools if tool not in exclude2]

# Process man3 (add extra path for ORD messages)
SRC_DIR = "../src"
DEST_DIR3 = "./md/man3"
exclude = [
    "sta"
]  # sta excluded because its format is different, and no severity level.
docs3 = [f"{SRC_DIR}/{tool}/messages.txt" for tool in tools if tool not in exclude]
docs3.append("../messages.txt")


def man2(path=DEST_DIR2):
    for doc in docs2:
        if not os.path.exists(doc):
            print(f"{doc} doesn't exist. Continuing")
            continue
        man2_translate(doc, path)


def man2_translate(doc, path):
    with open(doc) as f:
        text = f.read()
        # new function names (reading tcl synopsis + convert gui:: to gui_)
        func_names = extract_tcl_command(text)
        func_names = ["_".join(s.lower().split()) for s in func_names]
        func_names = [s.replace("::", "_") for s in func_names]

        # function description
        func_descs = extract_description(text)

        # synopsis content
        func_synopsis = extract_tcl_code(text)

        # arguments
        func_options, func_args = extract_arguments(text)

        print(f"{os.path.basename(doc)}")
        print(
            f"""Names: {len(func_names)},\
        Desc: {len(func_descs)},\
        Syn: {len(func_synopsis)},\
        Options: {len(func_options)},\
        Args: {len(func_args)}"""
        )
        assert (
            len(func_names)
            == len(func_descs)
            == len(func_synopsis)
            == len(func_options)
            == len(func_args)
        ), f"""Counts for all 5 categories must match up.\n
            {func_names}\n
            {func_descs}\n
            {func_synopsis}\n
            {func_options}\n
            {func_args}\n
            """

        for func_id in range(len(func_synopsis)):
            manpage = ManPage()
            manpage.name = func_names[func_id]
            manpage.desc = func_descs[func_id]
            manpage.synopsis = func_synopsis[func_id]
            if func_options[func_id]:
                # convert it to dict
                # TODO change this into a function. Or subsume under option/args parsing.
                switches_dict = {}
                for line in func_options[func_id]:
                    key, val = parse_switch(line)
                    switches_dict[key] = val
                manpage.switches = switches_dict

            if func_args[func_id]:
                # convert it to dict
                args_dict = {}
                for line in func_args[func_id]:
                    key, val = parse_switch(line)
                    args_dict[key] = val
                manpage.args = args_dict

            manpage.write_roff_file(path)
    print("Man2 successfully compiled.")


def man3(path=DEST_DIR3):
    for doc in docs3:
        print(f"Processing {doc}")
        if not os.path.exists(doc):
            print(f"{doc} doesn't exist. Continuing")
            continue
        man3_translate(doc, path)


def man3_translate(doc, path):
    with open(doc) as f:
        for line in f:
            parts = line.split()
            module, num, message, level = (
                parts[0],
                parts[1],
                " ".join(parts[3:-2]),
                parts[-2],
            )
            manpage = ManPage()
            manpage.name = f"{module}-{num}"
            if "with-total" in manpage.name:
                print(parts)
                exit()
            manpage.synopsis = "N/A."
            manpage.desc = f"Type: {level}\n\n{message}"
            manpage.write_roff_file(path)

    print("Man3 successfully compiled.")


if __name__ == "__main__":
    man2()
    man3()
