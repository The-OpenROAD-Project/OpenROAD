import os
import glob
import re

# Essentially, anything that exists in help must exist in man AND Readme.md
# Objective for each Tcl: try to parse sta::define_cmd_args {} <= end
# sta::parse_key_args ... keys {}... flags{} <-end 

# Help pattern counts (correct ones are listed)
# ant 1, fin 1, mpl 2, dbsta/readverilog 3, ifp 3, utl 1
# dst 3 (no readme), tap 5, grt 13, gui 7 (problematic..)
# pdn 9, psm 4, stt 1 (no readme), rsz 17
# IOPlacer.tcl missing the sta::define_cmd_args for no args functions.
# rcx 9, pad.tcl what about sta::define_hidden_cmd_args
# upf 8 dpl 1 par 6 mpl2 3 gpl 2 rmp 1
# dft.tcl does not have any sta::define_cmd_args!

# TODO: THose commands inside namespace eval sta need special regex. define_cmd_args. (e.g. dbsta/dbsta)

# Proc pattern counts

# Man pattern counts
#

for path in glob.glob("../../../src/*/src/*tcl"):
    # special handling for pad, since it has 3 Tcls.
    #if "pdn.tcl" not in path: continue
    if "ICeWall" in path or "PdnGen" in path: 
        continue

    help_pattern = r'sta::define_cmd_args\s+"(.*?)"\s*{([^}]*)}'
    proc_pattern = r''

    with open(path) as f:
        content = f.read()
        matches = re.findall(help_pattern, content, re.DOTALL)
        print(path, len(matches))
        for match in matches:
            help_cmd, help_args = match[0], match[1]
            #print(help_cmd)
            #print(help_args)

        if not matches: print('not found.')
