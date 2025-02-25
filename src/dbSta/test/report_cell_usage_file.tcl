# report_cell_usage 
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty "Nangate45/Nangate45_typ.lib"
read_def "report_cell_usage.def"

set file_name [make_result_file report_cell_usage_file.rpt]
report_cell_usage -file $file_name -stage test
diff_files $file_name report_cell_usage_file.rptok

