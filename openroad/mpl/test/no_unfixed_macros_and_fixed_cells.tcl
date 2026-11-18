# Test if the macro placement feasibility error for fixed standard cells inside
# the macro placement area won't throw unless there are macros to be placed.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/macro_only.lef"

read_def "./testcases/no_unfixed_macros.def"

set_thread_count 0

place_inst -cell BUF_X1 -name fixed_buf -orient R0 -status FIRM -loc "20 20"
rtl_macro_placer
