source "helpers.tcl"

set test_name emap

define_corners fast slow
set lib_files_slow {\
  ./asap7/asap7sc7p5t_AO_RVT_SS_nldm_211120.lib.gz \
  ./asap7/asap7sc7p5t_INVBUF_RVT_SS_nldm_220122.lib.gz \
  ./asap7/asap7sc7p5t_OA_RVT_SS_nldm_211120.lib.gz \
  ./asap7/asap7sc7p5t_SEQ_RVT_SS_nldm_220123.lib \
  ./asap7/asap7sc7p5t_SIMPLE_RVT_SS_nldm_211120.lib.gz\
}
set lib_files_fast {\
  ./asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz \
  ./asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz \
  ./asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz \
  ./asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib \
  ./asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz\
}

foreach lib_file $lib_files_slow {
  read_liberty -corner slow $lib_file
}
foreach lib_file $lib_files_fast {
  read_liberty -corner fast $lib_file
}

read_lef ./asap7/asap7_tech_1x_201209.lef
read_lef ./asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_verilog ./gcd_asap7.v
link_design gcd
read_sdc ./gcd_asap7.sdc

set_layer_rc -layer M1 -resistance 7.04175E-02 -capacitance 1e-10
set_layer_rc -layer M2 -resistance 4.62311E-02 -capacitance 1.84542E-01
set_layer_rc -layer M3 -resistance 3.63251E-02 -capacitance 1.53955E-01
set_layer_rc -layer M4 -resistance 2.03083E-02 -capacitance 1.89434E-01
set_layer_rc -layer M5 -resistance 1.93005E-02 -capacitance 1.71593E-01
set_layer_rc -layer M6 -resistance 1.18619E-02 -capacitance 1.76146E-01
set_layer_rc -layer M7 -resistance 1.25311E-02 -capacitance 1.47030E-01
set_wire_rc -signal -resistance 3.23151E-02 -capacitance 1.73323E-01
set_wire_rc -clock -resistance 5.13971E-02 -capacitance 1.44549E-01

set_layer_rc -via V1 -resistance 1.72E-02
set_layer_rc -via V2 -resistance 1.72E-02
set_layer_rc -via V3 -resistance 1.72E-02
set_layer_rc -via V4 -resistance 1.18E-02
set_layer_rc -via V5 -resistance 1.18E-02
set_layer_rc -via V6 -resistance 8.20E-03
set_layer_rc -via V7 -resistance 8.20E-03
set_layer_rc -via V8 -resistance 6.30E-03

puts "-- Before --\n"
report_cell_usage
report_timing_histogram
report_checks
report_wns
report_tns

write_verilog_for_eqy $test_name before "None"

puts "-- After --\n"

resynth_emap -corner fast \
  -genlib_file ../../../third-party/mockturtle/experiments/cell_libraries/asap7.genlib \
  -target timings \
  -map_multioutput \
  -verbose

repair_timing

report_cell_usage
report_timing_histogram
report_checks
report_wns
report_tns

puts "-- eqy --\n"

set liberty_files [concat $lib_files_slow $lib_files_fast]
run_equivalence_test $test_name \
  -liberty_files $liberty_files \
  -remove_cells "None"
