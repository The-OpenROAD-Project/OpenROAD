include "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_verilog aes_nangate45.v
link_design aes_cipher_top
create_clock [get_ports clk] -name core_clock -period 0.5
clock_gating
write_verilog results/aes_nangate45_gated.v
diff_file aes_nangate45_gated.vok results/aes_nangate45_gated.v
