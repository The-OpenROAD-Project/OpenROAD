# Test if the macro gets placed in the guidance region
# on the right side of the die.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"

read_lef "./testcases/macro_only.lef"
read_liberty "./testcases/macro_only.lib"

read_def "./testcases/guides1.def"

set_io_pin_constraint -direction INPUT -region left:*

set_macro_guidance_region -macro_name MACRO_1 -region {49 0 149 100}
set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir]

set def_file [make_result_file guides1.def]
write_def $def_file

diff_files guides1.defok $def_file
