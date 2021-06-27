# repair_design max_slew hi fanout input port
source "helpers.tcl"
source "hi_fanout.tcl"
source "sky130hd/sky130hd.vars"

set def_filename [make_result_file "repair_slew8.def"]
write_hi_fanout_def2 $def_filename 250 reset \
  "r" "sky130_fd_sc_hd__dfrtp_1" "CLK" "RESET_B" 100000 \
  "met2" 1000

read_liberty $liberty_file
read_lef $tech_lef
read_lef $std_cell_lef
read_def $def_filename
create_clock -period 1 clk1

source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock  -layer $wire_rc_layer_clk
set_dont_use $dont_use

estimate_parasitics -placement

report_check_types -max_slew -max_cap -max_fanout

# This should work the same as if the reset input was buffered already.
repair_design

report_check_types -max_slew -max_cap -max_fanout
