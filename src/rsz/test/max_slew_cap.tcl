# common to repair_cap3, repair_slew9
source "helpers.tcl"
source "hi_fanout.tcl"

read_liberty sky130hd/sky130hd_tt.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef

set def_file [make_result_file "repair_slew1.def"]
# fanout chosen so max driver strength of 4 chosen attempting
# to normalize slews is just under slew and cap limits
write_hi_fanout_def1 $def_file 104 \
  "rdrv" "sky130_fd_sc_hd__o21ai_0" "" "Y" \
  "r" "sky130_fd_sc_hd__dfxtp_1" "" "D" 5000 \
  "met1" 1000

read_def $def_file
create_clock -period 1 clk1

set_wire_rc -layer met3
estimate_parasitics -placement

puts "Found [sta::max_slew_violation_count] slew violations"
puts "Found [sta::max_capacitance_violation_count] cap violations"
