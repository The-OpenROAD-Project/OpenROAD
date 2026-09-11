source "helpers.tcl"

set test_name emap_jpeg_sky130hd

define_corners fast slow
set lib_files_slow {\
  ./sky130/sky130_fd_sc_hd__ss_n40C_1v40.lib\
}
set lib_files_fast {\
  ./sky130/sky130_fd_sc_hd__ff_n40C_1v95.lib\
}

foreach lib_file $lib_files_slow {
  read_liberty -corner slow $lib_file
}
foreach lib_file $lib_files_fast {
  read_liberty -corner fast $lib_file
}

read_lef ./sky130/sky130hd.tlef
read_lef ./sky130/sky130hd_std_cell.lef

read_verilog ./jpeg_sky130hd.v
link_design jpeg_encoder
read_sdc ./jpeg_sky130hd.sdc

source sky130/sky130hd.rc
set_wire_rc -signal -layer met1
set_wire_rc -clock -layer met3

puts "-- Before --\n"
report_cell_usage
report_checks
report_wns
report_tns

puts "-- After emap --\n"

resynth_emap -scene fast \
  -map_multioutput

report_cell_usage
report_timing_histogram
report_checks
report_wns
report_tns

puts "-- After repair --\n"

repair_timing

report_cell_usage
report_timing_histogram
report_checks
report_wns
report_tns
