# PARALLEL_EVAL under -path_group judges its batches by the group's own slack,
# as LEGACY does in repair_timing_path_group4. Late req_msg data makes in2reg
# the worst path at every violating reg2reg endpoint, so the design WNS and
# TNS barely see the reg2reg repair; a verdict on them kept batches that made
# reg2reg worse.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def

create_clock -name core_clock -period 0.55 [get_ports clk]
set_input_delay -clock core_clock 0.05 [all_inputs -no_clocks]
set_input_delay -clock core_clock 0.50 [get_ports req_msg*]
set_output_delay -clock core_clock 0.1 [all_outputs]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

rsz::resolve_path_group reg2reg
rsz::resolve_path_group in2reg

foreach when {before after} {
  if { $when == "after" } {
    repair_timing -setup -path_group reg2reg -phases PARALLEL_EVAL
  }
  foreach path_group {reg2reg in2reg} {
    puts "-- $path_group $when"
    report_checks -path_group $path_group -path_delay max -digits 4 \
      -fields {} -group_path_count 1 -format summary
  }
}
