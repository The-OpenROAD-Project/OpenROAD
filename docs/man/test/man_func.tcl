source "helpers.tcl"
# man functionality test

# Objective: Run `man` to test these features
# - Running man with no set path. (TODO)
# - Setting a new manpath. This manpath probably leads to here.

## KIV: Need to find out interactively is the OR folders. 
## For now assume we are running in ~/OpenROAD/build

set output [man openroad -manpath "../../cat"]