# repair_design tristate drviers (N^2 issue)
source "helpers.tcl"
source "hi_fanout.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib

set def_file [make_result_file "repair_design3.def"]
write_hi_fanout_def1 $def_file 1000 \
  "load" "sky130_fd_sc_hd__buf_1" "" "A" \
  "tri" "sky130_fd_sc_hd__ebufn_1" "" "Z" 5000 \
  "met1" 1000

read_def $def_file

source sky130hd/sky130hd.rc
set_wire_rc -layer met2
estimate_parasitics -placement
# force a max cap violation at the shorted tristate net
set_load 1 net0
repair_design
