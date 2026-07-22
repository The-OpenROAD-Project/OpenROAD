source "helpers.tcl"

set test_name gcd_annealing2

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


puts "-- Before --\n"
report_cell_usage
report_timing_histogram
report_checks
report_wns
report_tns
write_verilog_for_eqy $test_name before "None"

puts "-- After --\n"

resynth_annealing -corner fast -temp 1e-10 -seed 66
report_timing_histogram
report_cell_usage
report_checks
report_wns
report_tns

set liberty_files [concat $lib_files_slow $lib_files_fast]
run_equivalence_test $test_name \
  -liberty_files $liberty_files \
  -remove_cells "None"
