"""
Usage: 
cd <OPENROAD_ROOT>/docs/manpages
python md_roff_compat.py
"""

import re
import io
import datetime

# list of edited docs
SRC_DIR = "./md/man2"
# problematic: gpl, grt, gui, ifp, mpl2, pdn, psm
# psm: need to handle the table wrt the #### Options, not all tables. 
# also you need to change the ### FUNCTION_NAME parsing. Sometimes the 
#    function name could be something weird like `diff_spef` or `pdngen`
#    so it would be better to have a more informative header for the RTD docs. 
# rmp: many level 3 headers
# sta: documentation is hosted elsewhere. (not currently in RTD also.)
# 
tools = ["ant", "cts", "dft", "dpl", "fin", "pad", "par", "ppl", "rsz",\
            "tap", "upf", "drt"]
#tools = ["drt"]
docs = [f"{SRC_DIR}/{tool}.txt" for tool in tools]

# identify key section and stored in ManPage class. 
class ManPage():
    def __init__(self):
        self.name = ""
        self.desc = ""
        self.synopsis = ""
        self.switches = {}
        self.args = {}
        self.datetime = datetime.datetime.now().strftime("%y/%m/%d")

    def write_roff_file(self):
        assert self.name, print("func name not set")
        assert self.desc, print("func desc not set")
        assert self.synopsis, print("func synopsis not set")
        # it is okay for a function to have no switches.
        #assert self.switches, print("func switches not set")
        filepath = f"{SRC_DIR}/{self.name}.md"
        with open(filepath, "w") as f:
            self.write_header(f)
            self.write_name(f)
            self.write_synopsis(f)
            self.write_description(f)
            self.write_options(f)
            self.write_arguments(f)
            self.write_placeholder(f) #TODO.
            self.write_copyright(f)
    
    def write_header(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"---\n")
        f.write(f"title: {self.name}(2)\n")
        f.write(f"author: Jack Luar (TODO@TODO.com)\n")
        f.write(f"date: {self.datetime}\n")
        f.write(f"---\n")

    def write_name(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"\n# NAME\n\n")
        f.write(f"{self.name} - {' '.join(self.name.split('_'))}\n")

    def write_synopsis(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"\n# SYNOPSIS\n\n")
        f.write(f"{self.synopsis}\n")


    def write_description(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"\n# DESCRIPTION\n\n")
        f.write(f"{self.desc}\n")

    def write_options(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"\n# OPTIONS\n")
        if not self.switches:
            f.write(f"\nThis command has no switches.\n")
        for key, val in self.switches.items():
            f.write(f"\n`{key}`: {val}\n")

    def write_arguments(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"\n# ARGUMENTS\n")
        if not self.args:
            f.write(f"\nThis command has no arguments.\n")
        for key, val in self.args.items():
            f.write(f"\n`{key}`: {val}\n")


    def write_placeholder(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        # TODO: these are all not populated currently, not parseable from docs. 
        # TODO: Arguments can actually be parsed, but you need to preprocess the synopsis further. 
        sections = ["EXAMPLES", "SEE ALSO"]
        for s in sections:
            f.write(f"\n# {s}\n")

    def write_copyright(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"\n# COPYRIGHT\n\n")
        f.write(f"Copyright (c) 2024, The Regents of the University of California. All rights reserved.\n")

def extract_headers(text, level = 1):
    assert isinstance(level, int) and level >= 1
    pattern = r'^#{%d}\s+(.*)$' % level
    headers = re.findall(pattern, text, flags=re.MULTILINE)
    # TODO: Handle developer commands
    if "Useful Developer Commands" in headers: headers.remove("Useful Developer Commands")
    return headers

def extract_description(text):
    # this is so that it always tries to match the longer headers first, to disambiguate
    sorted_headers = sorted(extract_headers(text,3), key=len, reverse=True)
    headers = "|".join(re.escape(x) for x in sorted_headers)
    pattern = rf'### ({headers})(.*?)```tcl'
    custom_strings = re.findall(pattern, text, flags=re.DOTALL)
    return [custom_string[1].strip() for custom_string in custom_strings]

def extract_tcl_code(text):
    pattern = r'```tcl\s+(.*?)```'
    tcl_code_matches = re.findall(pattern, text, flags=re.DOTALL)
    # remove the last tcl match
    tcl_code_matches = [x for x in tcl_code_matches if "./test/gcd.tcl" not in x]
    return tcl_code_matches

def extract_arguments(text):
    # Goal is to extract all the text from the end of tcl code to the next ### header.
    # Returns options and arguments.
    level2 = extract_headers(text, 2)
    level3 = extract_headers(text, 3)

    # form these 2 regex styles. 
    # ### Header 1 {text} ### Header2; ### Header n-2 {text} ### Header n-1
    # ### Header n {text} ## closest_level2_header
    first = [rf'### ({level3[i]})(.*?)### ({level3[i+1]})'  for i in range(len(level3) - 1)]

    # find the next closest level2 header to the last level3 header.
    closest_level2 = [text.find(x) - text.find(level3[-1]) for x in level2]
    closest_level2_idx = [idx for idx, x in enumerate(closest_level2) if x > 0][0]

    second = [rf"### ({level3[-1]})(.*?)## ({level2[closest_level2_idx]})"]
    final_options, final_args = [], []
    for idx, regex in enumerate(first + second):
        match = re.findall(regex, text, flags = re.DOTALL)
        # get text until the next header
        a = match[0][1] 
        a = a[a.find("#"):]
        options = a.split("####")[1:]
        if not options:
            final_options.append([])
            final_args.append([])
            continue
        options, args = options[0], options[1:]
        final_options.append(extract_tables(options))
        tmp_arg = []
        for arg in args:
            tmp_arg.extend(extract_tables(arg))
        final_args.append(tmp_arg)
    return final_options, final_args

def extract_tables(text):
    # Find all lines that start with "|"
    table_pattern = r'^\s*\|.*$'
    table_matches = re.findall(table_pattern, text, flags=re.MULTILINE)

    # Exclude matches containing HTML tags
    table_matches = [table for table in table_matches if not re.search(r'<.*?>', table)]

    # Remove text containing switch 
    table_matches = [table for table in table_matches if "Switch Name" not in table]

    # Remove text containing "---"
    table_matches = [table for table in table_matches if "---" not in table]

    return table_matches

def parse_switch(text):
    # Find the index of the 1nd and last occurrence of "|". Since some content might contain "|"
    switch_name = text.split("|")[1]
    switch_name = switch_name.replace("`", "").strip()
    second_pipe_index = text.find("|", text.find("|") + 1)
    last_pipe_index = text.rfind("|")
    switch_description = text[second_pipe_index+1: last_pipe_index-1] 
    return switch_name, switch_description


if __name__ == "__main__":
    for doc in docs:
        text = open(doc).read()

        # function names
        func_names = extract_headers(text, 3)
        func_names = ["_".join(s.lower().split()) for s in func_names]

        # function description
        func_descs = extract_description(text)

        # synopsis content
        func_synopsis = extract_tcl_code(text)

        # arguments
        func_options, func_args = extract_arguments(text)

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

            manpage.write_roff_file()