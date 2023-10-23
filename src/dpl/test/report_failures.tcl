source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def report_failures.def
set json_file [make_result_file report_failures.json]
catch { detailed_placement -report_file_name $json_file } msg

set def_file [make_result_file report_failures.def]
write_def $def_file
diff_file $def_file report_failures.defok
diff_file $json_file report_failures.jsonok
