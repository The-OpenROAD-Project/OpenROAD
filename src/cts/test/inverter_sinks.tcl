# Clock inverters are macro sinks unless they drive a single register:
# inv_multi has two register loads, inv_gater drives a clock gater and
# inv_single drives one register.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def inverter_sinks.def

create_clock -period 5 clk

source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal1
set_wire_rc -clock -layer metal2

set_cts_config -root_buf CLKBUF_X3 -buf_list CLKBUF_X3

clock_tree_synthesis
