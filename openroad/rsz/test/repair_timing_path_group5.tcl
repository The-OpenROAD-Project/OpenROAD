# repair_timing -hold -path_group repairs the group's hold paths by the group's
# own slack, so worse hold paths of another group at the same endpoints are
# neither repaired nor reported.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def

# Early req_msg data makes in2reg the worst hold path at every register that
# reg2reg must repair. The reg2reg repair must match a run with that data late
# (-min input delay 0.30): 83 hold buffers, reg2reg hold slack 0.2012, and no
# warning about checks left within the margin, since in2reg is not repaired.
create_clock -name core_clock -period 0.65 [get_ports clk]
set_input_delay -clock core_clock 0.05 [all_inputs -no_clocks]
set_input_delay -clock core_clock -min -0.15 [get_ports req_msg*]
set_output_delay -clock core_clock 0.1 [all_outputs]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

rsz::resolve_path_group reg2reg
rsz::resolve_path_group in2reg

foreach when {before after} {
  if { $when == "after" } {
    repair_timing -hold -hold_margin 0.2 -path_group reg2reg
  }
  foreach path_group {reg2reg in2reg} {
    puts "-- $path_group $when"
    report_checks -path_group $path_group -path_delay min -digits 4 \
      -fields {} -group_path_count 1 -format summary
  }
}
