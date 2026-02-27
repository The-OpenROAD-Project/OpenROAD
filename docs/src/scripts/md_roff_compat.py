## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

# This code contains the scripts to convert the individual module READMEs
#  into individual functions for man2 and man3 level.

import os
from manpage import ManPage
from extract_utils import extract_tcl_command, extract_description
from extract_utils import extract_tcl_code, extract_arguments
from extract_utils import extract_tables, parse_switch


# Simplified extraction functions for EXAMPLES and SEE ALSO
def extract_global_examples(text):
    """
    Extract a single global EXAMPLES section from the entire markdown file.
    Returns the examples text or None if not found.
    """
    lines = text.split("\n")
    examples_text = ""
    in_examples = False

    for line in lines:
        if line.strip() == "#### EXAMPLES":
            in_examples = True
            continue
        elif line.strip().startswith("####") and in_examples:
            break
        elif in_examples:
            examples_text += line + "\n"

    return examples_text.strip() if examples_text.strip() else None


def extract_global_see_also(text):
    """
    Extract a single global SEE ALSO section from the entire markdown file.
    Returns a list of references or None if not found.
    """
    lines = text.split("\n")
    see_also_refs = []
    in_see_also = False

    for line in lines:
        if line.strip() == "#### SEE ALSO":
            in_see_also = True
            continue
        elif line.strip().startswith("####") and in_see_also:
            break
        elif in_see_also and line.strip():
            see_also_refs.append(line.strip())

    return see_also_refs if see_also_refs else None


# Undocumented manpages.
# sta: documentation is hosted elsewhere. (not currently in RTD also.)
# odb: documentation is hosted on doxygen.

tools = [
    "ant",
    "cts",
    "dbSta",
    "dft",
    "dpl",
    "drt",
    "dst",
    "fin",
    "gpl",
    "grt",
    "gui",
    "ifp",
    "mpl",
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

        # NEW: Extract examples and see also sections (simplified approach)
        # Assume one global EXAMPLES and SEE ALSO section per file that applies to all functions
        global_examples = extract_global_examples(text)
        global_see_also = extract_global_see_also(text)

        print(f"{os.path.basename(doc)}")
        print(f"""Names: {len(func_names)},\
        Desc: {len(func_descs)},\
        Syn: {len(func_synopsis)},\
        Options: {len(func_options)},\
        Args: {len(func_args)}""")
        print(f"Global Examples: {'Found' if global_examples else 'None'}")
        print(f"Global See Also: {'Found' if global_see_also else 'None'}")

        assert (
            len(func_names)
            == len(func_descs)
            == len(func_synopsis)
            == len(func_options)
            == len(func_args)
        ), f"""Counts for all 5 categories must match up.\n
            Names: {len(func_names)}\n
            Descs: {len(func_descs)}\n
            Synopsis: {len(func_synopsis)}\n
            Options: {len(func_options)}\n
            Args: {len(func_args)}\n
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

            # NEW: Add global examples and see also to each function (direct assignment)
            if global_examples:
                # Direct assignment to examples list
                manpage.examples = [
                    {"description": global_examples, "code": None, "output": None}
                ]

            if global_see_also:
                # Direct assignment to see_also list
                manpage.see_also = [
                    {"reference": ref, "section": None, "description": None}
                    for ref in global_see_also
                ]

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
            # man3 messages typically don't have examples or see also
            manpage.write_roff_file(path)

    print("Man3 successfully compiled.")


if __name__ == "__main__":
    man2()
    man3()
