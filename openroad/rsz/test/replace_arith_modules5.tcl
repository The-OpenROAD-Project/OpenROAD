# Swap arithmetic modules whose own outputs come back in through ports added
# by hierarchical buffering.
# u_alu: X[0] comes straight back in and X[1] through a buffer in the parent,
# so both ports are folded before the swap, and the buffer left without loads
# is removed.
# u_alu_out: the signal it brings back in leaves through an extra output port
# the swap target lacks, so nothing is folded and the swap is refused.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog replace_arith_modules5.v
link_design top -hier
create_clock -name clk -period 0.01 [get_ports clk]

set_debug_level ODB replace_design_check_sanity 1
puts "swapped: [replace_arith_modules -path_count 10]"

set block [ord::get_db_block]
foreach name {loopbuf_x1 loopbuf_g0} {
  if { [$block findInst $name] == "NULL" } {
    puts "$name: removed"
  } else {
    puts "$name: kept"
  }
}

set verilog_file [make_result_file replace_arith_modules5.v]
write_verilog $verilog_file
set stream [open $verilog_file r]
set verilog [read $stream]
close $stream
puts "module u_alu: [regexp -inline {ALU_\S+ u_alu\M} $verilog]"
puts "module u_alu_out: [regexp -inline {ALU_\S+ u_alu_out} $verilog]"
foreach name {loopX0 loopX1 x1_buf .g0in(} {
  if { [string first $name $verilog] < 0 } {
    puts "$name: absent from netlist"
  } else {
    puts "$name: present in netlist"
  }
}
# X[0] still reaches its flop outside, now driven by the new module.
puts "rx0/D: [get_full_name [get_nets -of_objects [get_pins rx0/D]]]"
report_checks -through rx0/D -format end
