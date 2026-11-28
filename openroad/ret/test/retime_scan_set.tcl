# A scan flop and a register with a live asynchronous set never move, even
# on failing paths; the same path behind a plain register slides (sky130hd).
source "helpers.tcl"
source "ret_report.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_scan_set.v
link_design top
create_clock -period 0.5 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4

foreach name {r1 r3} {
  set inst [[ord::get_db_block] findInst $name]
  puts "$name [[$inst getMaster] getName]"
}

report_new_cells
report_kept {r1 r3}
