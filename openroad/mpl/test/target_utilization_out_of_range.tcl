source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/macro_only.lef"

read_def "./testcases/io_constraints1.def"

catch { rtl_macro_placer -target_util 1.5 -report_directory [make_result_dir] }
