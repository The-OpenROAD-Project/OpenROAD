# Swap arithmetic modules that have ports added by hierarchical buffering.
# u_alu: the extra ports carry buffered copies of signals already entering
# through original ports, so they are folded before the swap; the buffer left
# without loads is removed and the one with another load is kept.
# u_alu_other: the extra port carries a different signal, so nothing is
# folded, the swap is refused and the instance is left unchanged.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog replace_arith_modules4.v
link_design top -hier
create_clock -name clk -period 0.01 [get_ports clk]

set_debug_level ODB replace_design_check_sanity 1
set swapped [replace_arith_modules -path_count 10]
puts "swapped: $swapped"
# u_alu is already swapped and u_alu_other is still refused: nothing to swap.
puts "swapped again: [replace_arith_modules -path_count 10]"

set block [ord::get_db_block]
foreach name {dupbuf_a0 dupbuf_b1 otherbuf_c} {
  if { [$block findInst $name] == "NULL" } {
    puts "$name: removed"
  } else {
    puts "$name: kept"
  }
}

set verilog_file [make_result_file replace_arith_modules4.v]
write_verilog $verilog_file
set stream [open $verilog_file r]
set verilog [read $stream]
close $stream
puts "module u_alu: [regexp -inline {ALU_\S+ u_alu\M} $verilog]"
puts "module u_alu_other: [regexp -inline {ALU_\S+ u_alu_other} $verilog]"
foreach name {dupA0 dupB1 a0_dup .other( c_buf} {
  if { [string first $name $verilog] < 0 } {
    puts "$name: absent from netlist"
  } else {
    puts "$name: present in netlist"
  }
}
