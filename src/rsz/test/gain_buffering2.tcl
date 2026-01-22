# gain buffering with centroid placement
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib

read_verilog gain_buffering1.v
link_design top

# Initialize floorplan
initialize_floorplan -site unithd -die_area {0 0 100 100} -core_area {10 10 90 90}

create_clock -name clk -period 1.0 [get_ports clk]

set_dont_use {sky130_fd_sc_hd__probe_*
    sky130_fd_sc_hd__lpflow_*
    sky130_fd_sc_hd__clkdly*
  	sky130_fd_sc_hd__dlygate*
  	sky130_fd_sc_hd__dlymetal*
  	sky130_fd_sc_hd__clkbuf_*
  	sky130_fd_sc_hd__bufbuf_*}

set_dont_touch _53_

# Spread cells to have a PLACED state
set i 0
foreach inst [get_cells -hierarchical *] {
  set name [get_full_name $inst]
  # Spread them out significantly to make centroid interesting
  # _69_ is a driver, others are loads.
  # Let's put _69_ at (10, 10) and loads at various places.
  if { $name == "_69_" } {
    place_inst -name $name -location "10 10"
  } else {
    set x [expr 20 + ($i % 5) * 15]
    set y [expr 20 + ($i / 5) * 10]
    place_inst -name $name -location "$x $y"
  }
  incr i
}

# Run parasitic estimation
estimate_parasitics -placement

report_checks -fields {fanout}
# repair_design -pre_placement triggers gain buffering
repair_design -pre_placement
report_checks -fields {fanout}

set block [ord::get_db_block]
check "gain1 location" {$block findInst "gain1" ; [$block findInst "gain1"] getLocation} \
  "39901 38016"
check "gain2 location" {$block findInst "gain2" ; [$block findInst "gain2"] getLocation} \
  "37651 29653"
check "gain3 location" {$block findInst "gain3" ; [$block findInst "gain3"] getLocation} \
  "45438 36052"

set test_name "gain_buffering2"
set verilog_file [make_result_file "${test_name}.v"]
write_verilog $verilog_file
diff_files ${test_name}.vok $verilog_file

exit_summary
