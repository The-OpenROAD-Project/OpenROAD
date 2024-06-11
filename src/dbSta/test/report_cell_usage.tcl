# report_cell_usage 
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty "Nangate45/Nangate45_typ.lib"
read_def "report_cell_usage.def"

sta::report_cell_usage