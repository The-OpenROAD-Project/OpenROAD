# Run macro placement and then the read-only report_macro_placement QoR
# command. Verifies the report runs and produces the expected metrics on a
# small all-macro design. report_macro_placement must NOT change placement.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45_tech.lef"
read_lef "./testcases/macro_only.lef"
read_liberty "./testcases/macro_only.lib"

read_verilog "./testcases/macro_only.v"
link_design "macro_only"

read_def "./testcases/macro_only.def" -floorplan_initialize

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir]

# Snapshot the placement, run the report, and confirm the report did not
# perturb the placement.
set def_before [make_result_file report_macro_placement1_before.def]
write_def $def_before

report_macro_placement

set def_after [make_result_file report_macro_placement1_after.def]
write_def $def_after

diff_files $def_before $def_after
