# sky130hs liberty cap axis != max_capacitance -> max cap violations
read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_liberty sky130hs/sky130hs_tt.lib

set def_file "max_cap.def"
# used to generate def
source ../../rsz/test/hi_fanout.tcl
write_hi_fanout_def1 $def_file 20 \
  "rdrv" "sky130_fd_sc_hs__buf_1" "" "X" \
  "r" "sky130_fd_sc_hs__dfxtp_1" "CLK" "D" 200000 \
  "met1" 1000
read_def $def_file

create_clock -period 5 clk1

source "sky130hs/sky130hs.rc"
set_wire_rc -signal -layer met2
set_wire_rc -clock  -layer met3
# same as met3
#set_wire_rc -clock  -capacitance [expr 1.4e-10 * 1e-6 * 1e+12] -resistance [expr 166800.0 * 1e-6 * 1e-3]
#set_wire_rc -clock  -capacitance [expr 100e-10 * 1e-6 * 1e+12] -resistance [expr 166800.0 * 1e-6 * 1e-3]

clock_tree_synthesis -root_buf sky130_fd_sc_hs__clkbuf_1 \
                     -buf_list sky130_fd_sc_hs__clkbuf_1

estimate_parasitics -placement
set_propagated_clock clk1
report_check_types -max_cap -max_slew -net [get_net -of_object [get_pin r1/CLK]]
