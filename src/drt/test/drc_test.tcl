source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def drc_test.def
set_thread_count [expr [exec getconf _NPROCESSORS_ONLN] / 4]
set drc_file [make_result_file drc_test.drc]
drt::check_drc -output_file $drc_file
diff_files $drc_file drc_test.drcok
