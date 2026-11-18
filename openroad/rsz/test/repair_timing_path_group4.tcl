# repair_timing -path_group judges its moves by the group's own slack, so worse
# paths of another group at the same endpoints do not hold the repair back.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def

# Late req_msg data makes in2reg the worst path at every violating reg2reg
# endpoint, through logic the reg2reg paths only meet at the register's input
# mux. Repairing reg2reg then leaves those endpoints' own worst slack where it
# was, and a repair that judged its moves by it would undo them. The reg2reg
# result must match a run with the inputs on time (input delay 0.05 on every
# input): -0.029 WNS, 16 violating endpoints.
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
    repair_timing -setup -path_group reg2reg
  }
  foreach path_group {reg2reg in2reg} {
    puts "-- $path_group $when"
    report_checks -path_group $path_group -path_delay max -digits 4 \
      -fields {} -group_path_count 1 -format summary
  }
}
