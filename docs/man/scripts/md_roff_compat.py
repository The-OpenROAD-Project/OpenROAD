"""
Usage: 
cd <OPENROAD_ROOT>/docs/manpages
python md_roff_compat.py
"""

import os
from manpage import ManPage
from extract_utils import extract_headers, extract_description, extract_tcl_code, extract_arguments, extract_tables, parse_switch

# also you need to change the ### FUNCTION_NAME parsing. Sometimes the 
#    function name could be something weird like `diff_spef` or `pdngen`
#    so it would be better to have a more informative header for the RTD docs. 
# TODO: New function name parsing. Instead of parsing level3 header. 
#       parse the func_name from the tcl itself. Then the level3 header can be used to
#       be the description of the function. E.g. `func_name - Useful Function Description` in the roff.
# sta: documentation is hosted elsewhere. (not currently in RTD also.)
# odb: documentation is hosted on doxygen. 
# 
tools = ["ant", "cts", "dbSta", "dft", "dpl", "dpo", "drt",\
        "dst", "fin", "gpl", "grt", "gui", "ifp", "mpl",\
        "mpl2", "odb", "pad", "par", "pdn", "ppl", "psm",\
        "rcx", "rmp", "rsz", "sta", "stt", "tap", "upf", "utl"]
# Process man2 (except odb and sta)
DEST_DIR2 = SRC_DIR = "./man/md/man2"
exclude = ["odb", "sta"]
docs2 = [f"{SRC_DIR}/{tool}.md" for tool in tools if tool not in exclude]

# Process man3 (add extra path for ORD messages)
SRC_DIR = "../src"
DEST_DIR3 = "./man/md/man3"
exclude = ["sta"] #sta excluded because its format is different, and no severity level.
docs3 = [f"{SRC_DIR}/{tool}/messages.txt" for tool in tools if tool not in exclude]
docs3.append("../messages.txt")

if __name__ == "__main__":
    for doc in docs2:
        if not os.path.exists(doc):
            print(f"{doc} doesn't exist. Continuing")
            continue
        text = open(doc).read()

        # function names (and convert gui:: to gui_)
        func_names = extract_headers(text, 3)
        func_names = ["_".join(s.lower().split()) for s in func_names]
        func_names = [s.replace("::", "_") for s in func_names]

        # function description
        func_descs = extract_description(text)

        # synopsis content
        func_synopsis = extract_tcl_code(text)

        # arguments
        func_options, func_args = extract_arguments(text)

        #print(f"Names: {len(func_names)}")
        #print(f"Desc: {len(func_descs)}")
        #print(f"Syn: {len(func_synopsis)}")
        #print(f"Options: {len(func_options)}")
        #print(f"Args: {len(func_args)}")

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

            manpage.write_roff_file(DEST_DIR2)
    count = 0
    for doc in docs3:
        if not os.path.exists(doc):
            print(f"{doc} doesn't exist. Continuing")
            continue
        print(f"Processing {doc}")
        with open(doc, 'r') as f:
            for line in f:
                count += 1
                parts = line.split()
                module, num, message, level = parts[0], parts[1],\
                                " ".join(parts[4:-2]), parts[-2]
                manpage = ManPage()
                manpage.name = f"{module}-{num}"
                if "with-total" in manpage.name: print(parts); exit()
                manpage.synopsis = "N/A."
                manpage.desc = f"Severity: {level}\n\n{message}"
                manpage.write_roff_file(DEST_DIR3)

                # For individual module folders.
                #module_path = os.path.join(DEST_DIR3, module)
                #os.makedirs(module_path, exist_ok = True)
                #manpage.write_roff_file(module_path)
    print(count)