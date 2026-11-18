# buffer_ports -output hands off tristate outputs
source helpers.tcl
read_liberty sky130hs/sky130hs_tt.lib
read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_def buffer_ports7.def

buffer_ports -output

set def_file [make_result_file buffer_ports7.def]
write_def $def_file
diff_files buffer_ports7.defok $def_file
