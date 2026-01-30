# Test if we error correctly when unfixed cells don't fit in the core area.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/macro_only.lef"

read_def "./testcases/unfixed_cells_dont_fit_in_core.def"

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir]
