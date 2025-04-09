# Example script, tee list of modules with keep attribute to a
# report file
source $::env(SCRIPTS_DIR)/yosys_load.tcl

tee -o $::env(REPORTS_DIR)/keep.txt ls A:keep_hierarchy=1
