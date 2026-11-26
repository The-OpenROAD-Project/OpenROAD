# Two paths from one gate into one register.  An edge of the graph holds one
# chain of registers, so the registers in front of the second path keep
# their depth: r1 stays, and k/D still reads it.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_reconverge.v
link_design top
create_clock -period 0.13 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
set k_data [[[ord::get_db_block] findInst k] findITerm D]
puts "k/D reads [[$k_data getNet] getName]"
