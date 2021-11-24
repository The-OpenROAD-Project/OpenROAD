# constant propgation thru registers
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog constant1.v
link_design top
sta::find_logic_constants
report_constant r1
report_constant r2
